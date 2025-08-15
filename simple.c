#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strlen
#include <unistd.h> // pwd, cd
#include <linux/limits.h> // path
 #include <sys/wait.h> // waitpid

// for suppressing specific unused variable warnings
#define UNUSED(x) (void)(x)

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

int execute_command(int argc, char **argv) {
    UNUSED(argc);

    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // child process lands here
        if (execvp(argv[0], argv) == -1) {
            perror("mmmsh");
        }
        return 1;
    } else if (pid < 0) {
        // arises from an error in forking
        perror("mmmsh");
    } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 0;
}

int main() {
    // tells you wether the input is coming from the terminal.
    int interactive = isatty(STDIN_FILENO);

    char *line;
    char** args;
    int argc;

    while(1){
        if (interactive) {
            fputs("mmmsh$ ", stdout);
            fflush(stdout);
        }

        line = read_line();
        
        // EOF: done
        if (line == NULL) break;

        args = parse_args(line, &argc);

        // No arguments: continue
        if (argc == 0 || args[0] == NULL){
            free(line);
            free(args);
            continue;
        } else if (strcmp(args[0], "exit") == 0) {
            printf("goodbye\n");
            free(line);
            free(args);
            break;
        }

        int success = execute_command(argc, args);
        
        free(line);
        free(args);

        if (success == 1){
            _exit(EXIT_FAILURE);
        }
    }
    
    return 0;
}