mmmsh: mmmsh.c
	gcc -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -g -o mmmsh mmmsh.c 