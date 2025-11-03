#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>

// for suppressing specific unused variable warnings
#define UNUSED(x) (void)(x)

#define PARENT_RETURN_VAL 0
#define CHILD_FAILED_EXECUTE 1
#define DID_NOT_EXECUTE 2
#define ERR_COULD_NOT_CREATE_PIPE 3

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
char **parse_args(char *line, int *argc){
    const int bufsize = 64;

    char **tokens = (char **)malloc(bufsize * sizeof (char*));
    char *token;
    char *delimiters = " ";
    int tokens_index = 0;

    token = strtok(line, delimiters);
    // printf("1st token - %s\n", token);

    while (token != NULL) {
        tokens[tokens_index] = token;
        tokens_index++;
        if (tokens_index == bufsize - 1){
            // this is flawed i beleive
            printf("error, too many tokens");
        }

        token = strtok(NULL, delimiters);
        // printf("token - %s\n", token);
    }

    tokens[tokens_index] = NULL; // sentinel? I don't understand C array yet

    *argc = tokens_index;

    return tokens;
}

// 
// BUILTIN COMMANDS
// 

int echo(int argc, char **args){
    for (int i = 1; i < argc; i++){
        if (i > 1) putchar(' ');
        fputs(args[i], stdout);
    }
    printf("\n");
    return 0;
}

int print_working_directory(int argc, char **args){
    UNUSED(args);
    if (argc > 1){
        fputs("pwd: too mamy arguments", stdout);
    }
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd() error");
        return 1;
    }
    return 0;
}

int change_directory (int argc, char **args){
    if (argc == 1){
        if (chdir("/") != 0) {
            printf("Error in changing to home directory\n");
            return 1;
        }
    } else if (argc > 2) {
        printf("cd: too many arguments");
        return 1;
    } else if (chdir(args[1]) != 0) {
        printf("Error in changing directory");
        return 1;
    }
    return 0;
}


int (*built_in_commands[]) (int, char **) = {
  &echo,
  &print_working_directory,
  &change_directory
};

char* built_in_command_names[] = {
    "echo",
    "pwd",
    "cd"
};

// A single command to run
struct command {
    char** argv;
    int argc;
};

// takes an array of tokens from the command prompt, and chops them up into an array of commands
struct command* parse_pipeline(int argc, char **argv, int* num_commands){
    *num_commands = 0;
    int command_array_bufsize = 256; // this can grow dynamically
    int command_index = 0;
    
    // allocating a dynamically sized buffer
    struct command* commands = (struct command*)malloc((command_array_bufsize + 1) * sizeof(struct command));

    for (int i = 0; i < argc; i++){
        // Start: we're expecting a normal token
        if (strcmp(argv[i], "|") == 0){
            puts("unexpected token");
        } 
        
        int indiv_command_argv_bufsize = 256; // this can grow dynamically. excludes NULL
        
        struct command current_command;
        current_command.argv = (char **)malloc((indiv_command_argv_bufsize + 1) * sizeof(char *));
        current_command.argc = 0;
        
        int token_index = 0;
        while (i < argc && strcmp(argv[i], "|") != 0){
            current_command.argv[token_index] = argv[i];
            token_index++;
            // see if this command needs more space to store its tokens
            if (token_index == indiv_command_argv_bufsize){
                puts("REALLOCATING INDIV_CMD_ARRAY");
                indiv_command_argv_bufsize *= 2;
                current_command.argv = (char **)realloc(current_command.argv, (indiv_command_argv_bufsize + 1) * sizeof(char *));
            }
            i++;
        }
        current_command.argc = token_index;
        current_command.argv[token_index] = NULL; // silences valgrind warning
        commands[command_index] = current_command;
        command_index++;

        // We've read everything in the input stream. Done!
        if (i == argc){
            break;
        }
        
        // see if we've run out of space for commands
        if (command_index == command_array_bufsize){
            puts("REALLOCATING COMMAND_ARRAY_BUFSIZE");
            command_array_bufsize *= 2;
            commands = (struct command*)realloc(commands, (command_array_bufsize + 1) * sizeof(struct command));
        }
    }
    *num_commands = command_index;
    commands[command_index].argv = NULL;

    return commands;
}



int execute_full_user_input(int argc, struct command* command_list) {
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

    // multiple process: multiple pids to wait for
    pid_t pids[argc];

    for (int command_index = 0; command_index < argc; command_index++){
        struct command current_command = command_list[command_index];
        int pid = fork();
        if (pid == 0) {
            pids[command_index] = pid;
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

            // check builtin
            for (int i = 0; built_in_command_names[i]; i++){
                if (strcmp(current_command.argv[0], built_in_command_names[i]) == 0){
                    (*built_in_commands[i])(current_command.argc, current_command.argv);
                    _exit(EXIT_SUCCESS);
                }
            }
            // execute normally
            if (execvp(current_command.argv[0], current_command.argv) == -1) {
                perror("mmmsh");
            } 
            // execvp ONLY returns if an error occurred.
            _exit(EXIT_FAILURE);
        }
    }
    // parent closes all pipes
    for (int i = 0; i < argc - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // parent must wait for everything
    int status = 0;
    for (int i = 0; i < argc; i++)
        waitpid(pids[i], &status, 0);

    return PARENT_RETURN_VAL;
}



int main() {
    // tells you wether the input is coming from the terminal.
    int interactive = isatty(STDIN_FILENO);

    char *line;
    char** args;
    int argc;
    int should_exit = 0;

    while(1){
        if (interactive) {
            fputs("mmmsh$ ", stdout);
            fflush(stdout);
        }

        line = read_line();
        
        // EOF: done
        if (line == NULL)
            break;

        // collect all arguments
        args = parse_args(line, &argc);

        

        int result = -1;
        UNUSED(result); // i'm still deciding whether or not to keep it. long term, it makes sense to have it.

        // ignore the input if empty
        if (argc == 0 || !args[0]){
            result = DID_NOT_EXECUTE;
            // controversial. but I think it belongs here.
            goto early_exit;
        }

        // parse the arguments into an array of commands. 
        // If there is more than 1 command, they are implied to have a pipe operator between them
        int num_commands;
        struct command* command_list = parse_pipeline(argc, args, &num_commands);


        // handle the input
        if (strcmp(args[0], "exit") == 0) {
            puts("goodbye");
            should_exit = 1;
            goto early_exit;
        }

        result = execute_full_user_input(num_commands, command_list);

        // Cleanup
        for (int i = 0; i < num_commands; i++)
            free(command_list[i].argv);

        free(command_list);

        early_exit:
        free(args);
        free(line);

        if (should_exit)
            break;
    }
    
    return 0;
}
