#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strlen
#include <unistd.h> // pwd, cd
#include <linux/limits.h> // path
#include <sys/wait.h> // waitpid

// for suppressing specific unused variable warnings
#define UNUSED(x) (void)(x)
#define PARENT_RETURN_VAL 0
#define CHILD_FAILED_EXECUTE 1
#define DID_NOT_EXECUTE 2
#define ERR_COULD_NOT_CREATE_PIPE 3


// returns a newly allocated string
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

    char **tokens = (char **)malloc(bufsize * sizeof(char*));
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
    
    // allocating a static sized buffer for simplicity (and laziness)
    // if exceeded just give up and print an error 
    struct command* commands = (struct command*)malloc(command_array_bufsize * sizeof(struct command));

    for (int i = 0; i < argc; i++){
        
        // Start: we're expecting a normal token
        if (strcmp(argv[i], "|") == 0){
            puts("unexpected token");
        } 
        
        int indiv_command_argv_bufsize = 256; // this can grow dynamically
        
        struct command current_command;
        current_command.argv = (char **)malloc(indiv_command_argv_bufsize * sizeof(char *));
        current_command.argc = 0;
        
        int token_index = 0;
        while (i < argc && strcmp(argv[i], "|") != 0){
            current_command.argv[token_index] = argv[i];
            token_index++;
            // see if this command needs more space to store its tokens
            if (token_index == indiv_command_argv_bufsize){
                indiv_command_argv_bufsize *= 2;
                current_command.argv = (char **)realloc(current_command.argv, indiv_command_argv_bufsize * sizeof(char *));
            }
            i++;
        }
        current_command.argc = token_index;
        commands[command_index] = current_command;
        command_index++;

        // We've read everything in the input stream. Done!
        if (i == argc) 
            break;

        // Input stream still has a pipe. continue
        i++;
        
        // see if we've run out of space for commands
        if (command_index == command_array_bufsize){
            command_array_bufsize *= 2;
            commands = (struct command*)realloc(commands, command_array_bufsize * sizeof(struct command));
        }
    }
    *num_commands = command_index;

    return commands;
}

// void free_command_list(command* command_list){
//     for (int i = 0; command_list[i]; i++){
//         free(command_list[i]);
//     }
// }

// takes in a command_list of user input, and executes it
// recognized operators: |
int execute_full_user_input(int argc, struct command* command_list) {
    puts("entering execute_full_user_input");
    UNUSED(argc);

    // int num_pipes = 0;
    for (int i = 0; i < argc; i++)
        continue;
        // if (command_list[i]) num_pipes++;

    // INITIALIZE THE PIPES
    // for every command in the command_list, there is an input pipe and an output pipe.
    // the command at the beginning reads from stdin
    // the command at the end writes to stdout
    // everything in the middle writes/reads to/from its neighbor.
    // Currently, this structure only works with the pipe | operator.
    // in the future, I hope to implement more operators

    // int pipes[num_pipes][2];
    // for (int i = 0; i < num_pipes; i++){
    //     if (pipe(pipes[i]) < 0){
    //         return ERR_COULD_NOT_CREATE_PIPE;
    //     }
    // }


    pid_t pid;
    int status;


    // structure outline:
    // loop
    //  fork()
    //  if child:
    //      command to execute = i
    //  else
    //      continue


    for (int command_index = 0; command_index < argc; command_index++){
        struct command current_command = command_list[command_index];
        pid = fork();
        // Child process
        if (pid == 0) {
            
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
            return CHILD_FAILED_EXECUTE;
        } else if (pid < 0) {
            // arises from an error in forking
            perror("mmmsh");
            return CHILD_FAILED_EXECUTE;
        } else {
            // We're in the parent
            // Track the child, and wait for it to finish.
            do {
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }


    return PARENT_RETURN_VAL;
}

// int execute_single_command (struct command command)


// int execute_pipeline(int argc, char **argv){

// }



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
        if (line == NULL) break;

        // collect all arguments
        args = parse_args(line, &argc);

        

        int result = -1;

        // ignore the input if empty
        if (argc == 0 || !args[0]){
            result = DID_NOT_EXECUTE;
            // oooOooOOooo controversial. trust me it belongs here.
            goto cleanup;
        }

        for (int i = 0; i < argc; i++){
            printf("token %d: %s\n", i, args[i]);
        }


        // parse the arguments into an array of 'commands'.
        // these can be commands, or operators, such as |, >, >>
        int num_commands;
        struct command* command_list = parse_pipeline(argc, args, &num_commands);

        printf("num commands: %d\n", num_commands);

        // for (int i = 0; i < num_commands; i++){
        //     printf("    tok: %s\n", command_list[i].argv[0]);
        // }
        // puts("moving on to command");



        // handle the input
        if (strcmp(args[0], "exit") == 0) {
            puts("goodbye");
            should_exit = 1;
        } else {
            result = execute_full_user_input(num_commands, command_list);
        }

        cleanup:
        free(args);
        free(line);

        if (should_exit == 1){
            break;
        }
        // I feel like i should do something different here
        if (result == CHILD_FAILED_EXECUTE){
            // why exit here? why not just print error and start over?
            // _exit(EXIT_FAILURE);
            puts("error in executing");
        } 
        // else if (result == PARENT_RETURN_VAL){
        //     _exit(EXIT_SUCCESS);
        // }

    }
    
    return 0;
}