#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strlen

// returns a newly allocated string
char* read_line(void){
    char *buffer;
    size_t bufsize = 0;
    ssize_t len = getline(&buffer,&bufsize,stdin);

    printf("Read %zd characters", len);

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
    printf("You typed: '%s'\n",line);
}

int main() {
    printf("Welcome to my mini shell\n");
    
    // infinite loop to repeat what the user says.

    int should_exit = 0;
    int error = 0;

    char *line;
    while(should_exit == 0){
        line = read_line();

        if (line == NULL) break; // EOF; check what happens without 

        if (strcmp(line, "exit") == 0) {
            should_exit = 1;
        } else {
            repeat(line);
        }

        free(line);
    }

    printf("\n");
    return 0;
}