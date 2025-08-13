#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strlen

// returns a newly allocated string
char* read_line(void){
    char *buffer;
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
char **parse_args(char *line){
    const int bufsize = 64;

    char **tokens = (char **)malloc(bufsize * sizeof(char*));
    char *token;
    char *delimiters = " ";
    int tokens_index = 0;

    token = strtok(line, delimiters);

    while (token != NULL) {
        tokens[tokens_index] = token;
        tokens_index++;
        if (tokens_index >= bufsize){
            printf("error, too many tokens");
        }

        token = strtok(NULL, delimiters);
    }

    return tokens;
}

void repeat(char* line){
    printf("%s\n",line);
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
    // int error = 0;

    char *line;
    while(should_exit == 0){
        printf("mmmsh$ ");

        line = read_line();

        if (line == NULL) break; // EOF; check what happens without 

        if (strcmp(line, "exit") == 0) {
            should_exit = 1;
        }


        char** args = parse_args(line);

        int i = 0;
        while (args[i] != NULL){
            repeat(args[i]);
            i++;
        }
        
        // repeat(line);
        free(line);
    }

    return 0;
}