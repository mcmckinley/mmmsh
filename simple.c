#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strlen

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
int pop_argument(char **args, char **out) {
    if (!args || !out) return 1;

    *out = args[0];

    // Shift everything left
    for (size_t i = 0; args[i]; i++) {
        args[i] = args[i + 1];
    }
    return 0;
}

// int change_directory(char** args) {

// }

void echo_one_arg(char **args){
    char* output;
    if (pop_argument(args, &output) == 1){
        printf("echo: requires 1 argument\n");
    } else {
        repeat(output);
    }
}



int main() {
    printf("Welcome to my mini shell\n");

    // pid_t p = fork();
    // if(p<0){
    //   perror("fork fail");
    //   exit(1);
    // }
    // printf("Hello world!, process_id(pid) = %d \n",getpid());

    
    // infinite loop to repeat what the user says.

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
        if (argc == 0 || pop_argument(args, &first_arg) == 1){
            free(line);
            free(args);
            continue;
        }

        if (strcmp(line, "exit") == 0) {
            printf("goodbye\n");
            should_exit = 1;
        }
        else if (strcmp(first_arg, "cd") == 0){
            printf("hi\n");
        }
        else if (strcmp(first_arg, "echo") == 0){
            echo_one_arg(args);
        }
        
        free(line);
        free(args);
    }
    
    return 0;
}