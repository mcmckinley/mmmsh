#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // strlen

// output (int): 0 if success. 1 if error
int read_and_repeat(void){
    char *buffer;
    size_t bufsize = 32;
    ssize_t characters;

    buffer = (char *)malloc(bufsize * sizeof(char));

    if (buffer == NULL) {
        perror("Unable to allocate buffer");
        return 1;
    }

    printf("mmmsh$ ");
    characters = getline(&buffer,&bufsize,stdin);
    printf("%zu characters recieved.\n",characters);


    size_t len = strlen(buffer);
    if (len) buffer[len - 1] = '\0';


    printf("You typed: '%s'\n",buffer);

    free(buffer);

    return 0;
}

int main() {
    printf("Welcome to my mini shell\n");

    printf("Sum of %d and %d is %d\n", 99, 1,
        99 + 1);
    
    // infinite loop to repeat what the user says.

    int status = read_and_repeat();

    if (status == 0){
        printf("Exited successully\n");
    } else {
        printf("Exited with error\n");
    }
    
    printf("\n");
    return 0;
}