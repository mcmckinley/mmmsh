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

// 
// EXECUTING A COMMAND
// 

// returns CHILD_FAILED_EXECUTE if the command failed.
// else, returns PARENT_RETURN_VAL


// Describes an element in the pipeline.
// Can be one of two things:
// 1. a command, with argv and argc
// 2. an operator, like | > >>
struct command {
    char** argv;
    int argc;
    int is_special_operator;
    char special_operator;
};

// takes an array of tokens from the command prompt, and chops them up into an array of commands
struct command* parse_pipeline(char **argv, int* num_commands){
    *num_commands = 0;
    const int command_bufsize = 256;
    int argv_index = 0;
    
    // allocating a static sized buffer for simplicity (and laziness)
    // if exceeded just give up and print an error 
    struct command* commands = (struct command*)malloc(command_bufsize * sizeof(struct command));
    
    
    for (int i = 0; i < command_bufsize; i++){
        if (!argv[argv_index]){
            *num_commands = i;
            break;
        }

        // create a command struct to be appended to the commands list
        struct command current_command;
        current_command.argv = (char **)malloc(command_bufsize * sizeof(char *));
        current_command.argc = 0;

        // fill the argv of the command struct
        if (strcmp(argv[argv_index], "|") != 0){
            printf("normal token found @ %d\n", argv_index);
            printf("%s\n", argv[argv_index]);

            // read in argc and argv of current command
            current_command.is_special_operator = 0;
            int token_index = 0;
            while (argv[argv_index] && strcmp(argv[argv_index], "|") != 0){
                current_command.argv[token_index] = argv[argv_index];
                argv_index++;
                token_index++;
            }
            current_command.argc = token_index;
        } 
        // Token is an operator
        else {
            printf("special op found @ %d\n", argv_index);
            current_command.special_operator = '|';
            current_command.is_special_operator = 1;
            current_command.argc = 1;
            current_command.argv = NULL;
            argv_index++;
        }

        if (i == command_bufsize - 1){
            perror("pipeline reached capacity");
        }

        commands[i] = current_command;
    }

    return commands;
};

// void free_command_list(command* command_list){
//     for (int i = 0; command_list[i]; i++){
//         free(command_list[i]);
//     }
// }

// takes in a string of user input, and executes it.
// recognizes 
int execute_full_user_input(int argc, char **argv) {
    UNUSED(argc);

    pid_t pid;
    int status;


    // loop
    //  fork()
    //      if child:
            //  command to execute = i
    //  else
        // continue


    pid = fork();

    if (pid == 0) {
        // child process lands here
        for (int i = 0; built_in_command_names[i]; i++){
            if (strcmp(argv[0], built_in_command_names[i]) == 0){
                (*built_in_commands[i])(argc, argv);
                return PARENT_RETURN_VAL;
            }
        }
        if (execvp(argv[0], argv) == -1) {
            perror("mmmsh");
        }
        return CHILD_FAILED_EXECUTE;
    } else if (pid < 0) {
        // arises from an error in forking
        perror("mmmsh");
    } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return PARENT_RETURN_VAL;
};

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

        

        int result;

        // ignore the input if empty
        if (argc == 0 || !args[0]){
            result = DID_NOT_EXECUTE;
            goto cleanup;
        }

        for (int i = 0; i < argc; i++){
            printf("token %d: %s\n", i, args[i]);
        }


        // parse the arguments into an array of 'commands'.
        // these can be commands, or operators, such as |, >, >>
        int num_commands;
        struct command* command_list = parse_pipeline(args, &num_commands);

        printf("num commands: %d\n", num_commands);

        for (int i = 0; i < num_commands; i++){
            printf("argc: %d\n", command_list[i].argc);
            if (command_list[i].is_special_operator){
                printf("    special op: %c\n", command_list[i].special_operator);
            } else {
                printf("    normal tok: %s\n", command_list[i].argv[0]);
            }
        }
        puts("ending prematurely");
        result = DID_NOT_EXECUTE;
        goto cleanup;



        // handle the input
        if (strcmp(args[0], "exit") == 0) {
            puts("goodbye");
            should_exit = 1;
        } else {
            result = execute_full_user_input(argc, args);
        }

        cleanup:
        free(args);
        free(line);

        if (should_exit == 1){
            break;
        }
        if (result == CHILD_FAILED_EXECUTE){
            _exit(EXIT_FAILURE);
        }
    }
    
    return 0;
}