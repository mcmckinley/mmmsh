# mmmsh — a tiny shell in C

<video src="https://github.com/user-attachments/assets/851df9fb-9ba6-4333-acb1-f6fe536ff5da" autoplay loop muted playsinline></video>

A UNIX-like shell that supports basic command syntax and pipelines. I made this to learn low-level programming in C.


## Features

- REPL with prompt (`mmmsh$`) when attached to a TTY
- Built-ins: `cd`, `pwd`, `echo`
- External commands via `execvp` (PATH search)
- Pipelines: `cmd1 | cmd2 | ... | cmdN`
- Graceful EOF handling (`Ctrl-D`)

## Out of scope (for now...)

- **Rolling pipes**
    - The current execution system opens all pipes up front, creating large VLAs on the stack. A better method would be a "rolling" one, which only has one active at a time. If I decide to upgrade this project I'll do this next.
- **Command history (↑/↓)**
    - This is a little more complicated to implement than what I was hoping. I want to keep this project relatively simple and easy to understand for a beginner, so I'm leaving it out.
- **Job control (`&`, `fg`, `wait`)**
- **Redirection (`>`, `<`, `2>`, `>>`)**
- **Quoting/escaping**
- **Variables**
- **Signal policies: running CTRL + C kills the parent**
    - This adds 50-100 lines of complex code, not worth it for this simple shell.
- **Better parsing flexibility**
    - For example: `ls | grep readme.md` succeeds but `ls|grep readme.md` fails. 
- **Expansions** such as `~`, `*`


## Build and run
Local:
```
make
./mmmsh
```
Docker:
```
./build.sh
./run.sh          # drops you in /work; then:
make && ./mmmsh
```

#### Example commands
Pipelines proof of concept:
```
ls -la | grep readme.md
```


## Program overview/flow

1. **Prompt `mmmsh(x)$`and read input** from user. *x* represents the status of the previous executed command.
2. **Tokenize input into commands**, e.g., (1) `ls` (2) `grep readme`. The pipe operator is implied to be between each element.
3. **Check if `exit`/`cd`**: these commands affect the state of the parent, so they have to be checked in the parent. (All other commands run within child processes.)
4. **Initialize pipes**, all upfront using `pipe()`. 
5. **For each command, `fork()`**:
    (a) if child: update and close pipes, then call `execvp()` with the command. If it returns, exit with error code
    (b) if parent: save pid for later
6. **Cleanup and wait**: `close()` all pipes. `waitpid()` for each process to finish.
7. **Save exit status and repeat**: return to step 1, showing the user the status of the last command

