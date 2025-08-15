#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strlen
#include <unistd.h> // pwd, cd
#include <linux/limits.h> // path

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

void repeat(char* line){
    printf("%s\n",line);
}

// split up a string into a string array, delimited by spaces
char **parse_args(char *line, int *argc){
    const int bufsize = 64;

    char **tokens = (char **)malloc(bufsize * sizeof(char*));
    char *token;
    char *delimiters = " ";
    int tokens_index = 0;

    token = strtok(line, delimiters);

    while (token != NULL) {
        tokens[tokens_index] = token;
        tokens_index++;
        if (tokens_index == bufsize - 1){
            printf("error, too many tokens");
        }

        token = strtok(NULL, delimiters);
    }

    tokens[tokens_index] = NULL; // sentinel? I don't understand C array yet

    *argc = tokens_index;

    return tokens;
}



// returns 0 on success, 1 on failure
// this is likely useless, and i will remove it.
int get_command(char **args, char **out) {
    if (!args || !out) return 1;

    *out = args[0];

    return 0;
}

// 
// BUILTIN COMMANDS
// 

void echo(int argc, char **args){
    for (int i = 1; i < argc; i++){
        if (i > 1) putchar(' ');
        fputs(args[i], stdout);
    }
    printf("\n");
}

int print_working_directory(int argc, char **args){
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
    if (argc < 2){
        printf("cd: requires one argument (yeah i know, this is wrong...)\n");
        return 1;
    } else if (argc > 2) {
        printf("cd: too many arguments");
        return 1;
    }

    if (chdir(args[1]) != 0) {
        printf("Error in changing directory");
        return 1;
    }
    return 0;
}

// 
// EXECUTING A COMMAND
// 

int fork_and_execute(int argc, char **argv) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(argv[0], argv) == -1) {
            perror("mmmsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("mmmsh");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

  return 1;
}



int main() {
    printf("Welcome to my mini shell\n");


    int should_exit = 0;
    char *line;
    char** args;
    int argc;
    while(should_exit == 0){
        printf("mmmsh$ ");

        line = read_line();
        
        // EOF: done
        if (line == NULL) break;

        args = parse_args(line, &argc);
        char* first_arg;

        // No arguments: continue
        if (argc == 0 || get_command(args, &first_arg) == 1){
            free(line);
            free(args);
            continue;
        }

        if (strcmp(line, "exit") == 0) {
            printf("goodbye\n");
            should_exit = 1;
        }
        else if (strcmp(first_arg, "echo") == 0){
            echo(argc, args);
        }
        else if (strcmp(first_arg, "pwd") == 0){
            print_working_directory(argc, args);
        }
        else if (strcmp(first_arg, "cd") == 0){
            change_directory(argc, args);
        } else if (strcmp(first_arg, "ls") == 0){
            fork_and_execute(argc, args);
        }

        
        free(line);
        free(args);
    }
    
    return 0;
}