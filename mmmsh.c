#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <errno.h>

// for suppressing specific unused variable warnings
#define UNUSED(x) (void)(x)

#define ERR_COULD_NOT_CREATE_PIPE 3
#define ERR_FORKING 4


// Read a line of input: return a null-terminated string
char* read_line(void){
    char *buffer = NULL;
    size_t bufsize = 0;
    ssize_t len = getline(&buffer,&bufsize,stdin);

    if (len == -1) {
        free(buffer);
        return NULL;
    }

    // trim trailing newline
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    return buffer;
}

// split up a string into a string array, delimited by spaces
// char *line - the input string
// *argc      - set to the number of args in char* line
// returns char** - each token from the input string that is separated by spaces/tabs
char **parse_args(char *line, int *argc){
    int bufsize = 64;
    char **tokens = (char **)malloc(bufsize * sizeof (char*));
    int tokens_index = 0;

    for (char *tok = strtok(line, " \t"); tok; tok = strtok(NULL, " \t")) {
        if (tokens_index + 1 >= bufsize){
            bufsize *= 2; 
            tokens = realloc(tokens, bufsize * sizeof *tokens);
        }
        tokens[tokens_index++] = tok;
    }
    tokens[tokens_index] = NULL;

    *argc = tokens_index;
    return tokens;
}

// Stateful builtin commands

int change_directory(int argc, char **args){
    const char *target = (argc == 1) ? getenv("HOME") : args[1];
    if (!target) { 
        fprintf(stderr, "cd: HOME not set\n");
        return 1;
    }
    if (argc > 2) { 
        fprintf(stderr, "cd: too many arguments\n");
        return 1;
    }
    if (chdir(target) != 0) {
        perror("cd");
        return 1;
    }
    return 0;
}

// Interpreting the input

// Represents a command and its arguments
//      argv - the arguments to be put into execvp
//      argc - number of arguments
struct command {
    char** argv;
    int argc;
};

// takes the chopped input from parse_args, and further parses them into a pipeline
// input:
//      int num_tokens - the number of individual tokens
//      char** argv - the chopped input from parse_args
// updated by reference:
//      int* num_commands - set to the number of commands
//      int* failed_to_parse - if set to 1, the parsing failed, and the output should be disregarded
// return:
//      command* - the resulting array of commands
struct command* parse_pipeline(int num_tokens, char **argv, int* num_commands, int* failed_to_parse){
    UNUSED(failed_to_parse);

    *num_commands = 0;
    int command_array_bufsize = 256; // this can grow dynamically
    int command_index = 0;
    
    // allocating a dynamically sized buffer
    struct command* commands = (struct command*)malloc((command_array_bufsize + 1) * sizeof(struct command));

    // tracks whether a pipe was the most recent thing received in the input stream. 
    // If this is set, and the program sees another, it fails
    // int just_received_pipe = 1;

    for (int i = 0; i < num_tokens; i++){
        // Start: we're expecting a normal token
        if (strcmp(argv[i], "|") == 0){
            fprintf(stderr, "unexpected token: '|'\n");
            *failed_to_parse = 1;
            *num_commands = command_index;
            return commands;
        }
        
        int indiv_command_argv_bufsize = 256; // this can grow dynamically. excludes NULL
        
        struct command current_command;
        current_command.argv = (char **)malloc((indiv_command_argv_bufsize + 1) * sizeof(char *));
        current_command.argc = 0;
        
        int token_index = 0;
        while (i < num_tokens && strcmp(argv[i], "|") != 0){
            current_command.argv[token_index] = argv[i];
            token_index++;
            // see if this command needs more space to store its tokens
            if (token_index == indiv_command_argv_bufsize){
                indiv_command_argv_bufsize *= 2;
                current_command.argv = (char **)realloc(current_command.argv, (indiv_command_argv_bufsize + 1) * sizeof(char *));
            }
            i++;
        }
        current_command.argc = token_index;
        current_command.argv[token_index] = NULL; // silences valgrind warning
        commands[command_index] = current_command;
        command_index++;

        // check if there's a trailing pipe
        if (i == num_tokens - 1) {               // this means the '|' was last
            fprintf(stderr, "unexpected token at end: '|'\n");
            *failed_to_parse = 1;
            *num_commands = command_index;
            return commands;
        }

        // We've read everything in the input stream. Done!
        if (i == num_tokens){
            break;
        }
        
        // see if we've run out of space for commands
        if (command_index == command_array_bufsize){
            command_array_bufsize *= 2;
            commands = (struct command*)realloc(commands, (command_array_bufsize + 1) * sizeof(struct command));
        }
    }
    *num_commands = command_index;
    commands[command_index].argv = NULL;

    return commands;
}


// Executes a command array from parse_pipeline
// input:
//      int argc - the number of commands
//      command* command_list - the list of commands
// return:
//      int - exit status of execution / last command

int execute_pipeline(int argc, struct command* command_list) {
    if (argc <= 0) return 0;

    // for every command in the command_list, there is an input pipe and an output pipe.
    // the command at the beginning reads from stdin
    // the command at the end writes to stdout
    // everything in the middle writes/reads to/from its neighbor.

    int pipes[argc][2];
    for (int i = 0; i < argc - 1; i++){
        if (pipe(pipes[i]) < 0){
            return ERR_COULD_NOT_CREATE_PIPE;
        }
    }

    int launched = 0;

    // multiple process: multiple pids to wait for
    pid_t pids[argc];

    for (int command_index = 0; command_index < argc; command_index++){
        struct command current_command = command_list[command_index];
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            // Read from previous pipe, not STDIN
            if (command_index != 0){
                dup2(pipes[command_index - 1][0], STDIN_FILENO);
            }
            
            // Write to next pipe, not STDOUT
            if (command_index < argc - 1){
                dup2(pipes[command_index][1], STDOUT_FILENO);
            }
            
            // in the child, we need to close every pipe, not just the one we just used
            for (int j = 0; j < argc - 1; j++){
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // error check
            if (current_command.argc == 0 || current_command.argv[0] == NULL) {
                fprintf(stderr, "command is NULL or has no args");
                _exit(EXIT_FAILURE);
            }

            // if execvp succeeds, the program here does not continue
            execvp(current_command.argv[0], current_command.argv);
            if (errno == ENOENT) _exit(127);
            perror("mmmsh");
            _exit(126);
        } else if (pid < 0) {
            // pid < 0 means that the fork failed. we need to clean up.
            // 1: close all pipes
            for (int j = 0; j < argc - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            // 2: wait for previous children to complete
            for (int k = 0; k < launched; k++)
                waitpid(pids[k], NULL, 0);

            // 3: special err code
            return ERR_FORKING;
        } else {
            // parent: save pid
            pids[command_index] = pid;
            launched++;
        }
    }
    // parent closes all pipes
    for (int i = 0; i < argc - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // parent must wait for everything.
    // also, track last command's status
    int status = 0, last_status = 0;
    for (int i = 0; i < argc; i++){
        pid_t w = waitpid(pids[i], &status, 0);
        if (w > 0 && i == argc - 1) {
            if (WIFEXITED(status))      last_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) last_status = 128 + WTERMSIG(status);
            else                          last_status = 1;  // fallback, don't consider other errors
        }
    }

    return last_status;
}

int main(void) {
    // tells you whether the input is coming from the terminal.
    int interactive = isatty(STDIN_FILENO);

    char *line;
    char** args;
    int argc;
    int last_status = 0;
    
    while(1){
        int status = 0;
        if (interactive) {
            printf("mmmsh(%d)$ ", last_status);
            fflush(stdout);
        }

        line = read_line();
        
        // EOF: done
        if (line == NULL)
            break;

        // collect all arguments
        args = parse_args(line, &argc);


        if (argc == 0 || !args[0])
            goto early_exit;

        // first, check for the two builtins, exit and cd:
        if (strcmp(args[0], "exit") == 0){
            if (argc > 2){
                fprintf(stderr, "exit: too many arguments\n");
                return 1;
            }
            int code = (argc == 2) ? atoi(args[1]) : last_status;
            exit(code);
        }
        if (strcmp(args[0], "cd") == 0){
            status = (*change_directory)(argc, args);
            goto early_exit;
        }
        
        // parse the arguments into an array of commands. 
        // If there is more than 1 command, they are implied to have a pipe operator between them
        int num_commands;
        int failed_to_parse = 0;
        struct command* command_list = parse_pipeline(argc, args, &num_commands, &failed_to_parse);

        if (failed_to_parse)
            goto parse_fail;
        
        // Execute!
        status = execute_pipeline(num_commands, command_list);

        // Check status
        switch (status) {
            case ERR_COULD_NOT_CREATE_PIPE: 
                fprintf(stderr, "error in pipe creation\n"); break;
            case ERR_FORKING: 
                fprintf(stderr, "error in forking\n"); break;
            case 127:
                fprintf(stderr, "error: command not found\n"); break;
            case 126:
                fprintf(stderr, "error: permission denied\n"); break;
            default: 
                break;
        }
        
        // Cleanup
        parse_fail:
        for (int i = 0; i < num_commands; i++)
            free(command_list[i].argv);
        free(command_list);
        
        early_exit:
        free(args);
        free(line);
        
        if (status >= 0)
            last_status = status;
    }
    
    return 0;
}
