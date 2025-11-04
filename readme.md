# mmmsh — a tiny shell in C

A minimal shell that supports basic commands and simple pipelines. I made this to learn systems programming in C.

## Features

- REPL with prompt (`mmmsh$`) when attached to a TTY
- Built-ins: `cd`, `pwd`, `echo`
- External commands via `execvp` (PATH search)
- Pipelines: `cmd1 | cmd2 | ... | cmdN`
- Graceful EOF handling (`Ctrl-D`)

## Out of scope (for now...)

- Command history / scrolling back with arrow keys
> this is a little more complicated to implement than I was hoping. I want to keep this project relatively simple and easy to understand for a beginner, so I'm leaving it out, for now
- Job control (`&`, `fg`, `wait`)
- Redirection (`>`, `<`, `2>`, `>>`)
- Quoting/escaping
- Variables
- A cleaner piping / redirection system
> The current one opens all pipes up front. A better method would be a "rolling" one, which opens them one at a time. It would look cleaner, and provide less opportunity for a vulnerability involving open file descriptors. 

## Build and run

```
make
./mmmsh
```
