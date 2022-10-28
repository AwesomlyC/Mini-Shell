# Mini-Shell

# Brief Description
A "mini" version of the Unix Shell using C programming language. 

# How-to-run
1. To run, download minishell.c into a directory
2. Compile and run minishell.c
3. That's all!

# Detailed Description
****Implementation Details****
This mini-shell has both built-in and external commands. The built-in commands are implemented using execv() and execvp(). These built-in commands consist of ```cd```, ```ls```, ```pwd```, and many more. On the other hand, some external commands you can run are ```./a.out (executable file)```.

When you enter a command into mini-shell, it first checks if its a built-in command
1. If the command is a built-in, it will run it using execv() or execvp(), depending on the user input.
2. If not, it will fork into a new process. Depending on the user input, the process can become a foreground process, in which the user waits for the process to be completed, or a background process, in which the user can enter a new command.

The user can send two signals to stop foreground processes such as

```ctrl+c (SIGINT)```

```ctrl+z (SIGTSTP)```

# Commands
The following commands that can be executed:

```cd <directory>```

```ls```

```pwd```

```jobs```

```clear```

```fg <job_id|pid>```

```bg <job_id|pid>```

```kill <job_id|pid>```

# Additional Feature
Mini-shell allows the user to include Input/output redirection, that also includes in both direction at the same time. The user can also specify if the output should be appended to their desired file.

```<```

```>```

```>>```

```(executable) < (file) >/>> file```
