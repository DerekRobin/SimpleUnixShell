# SH360.c

This is a simple Unix shell supporting output redirection and piping of up to three commands.
This was written for the operating systems course at the university of Victoria.

## A & B

In `main()` we read the .sh360rc and populate the local variables prompt and paths with the first line of .sh360rc and the remaining lines respectively through the use of `read_rc()`.

It is assumed that prompt cannot exceed 10 characters in length and that the each directory contained in .sh360rc will be at most 80 characters in length with a max of 10 directories, one per line.

We then enter an infintite for loop which prompts the user to do something. This can be exited by entering the command `exit`.

In the simple case of executing a single command, `main()` tokenizes the user input via `regular_tokenize()` (only 80 of those input characters will be copied into the `input` variable, greater than 80 characters may cause undefined behaviour). Then the program gets the path to the binary executable of the command which the user enters via `get_exec_path()`. Finally, the command is ran using a call to the `fork()` and `execve()` system calls which are wrapped in `run_command()`.

If the command entered consists of greather than seven tokens, then `Error: Number of arguments cannot exceed 7!` is printed to stderr.

If the command entered does not have a binary executable located in one of the directories found in .sh360rc, then `command: command not found` is printed to stderr.

## C

If the first token of the command is OR then we check to see if the input is of the proper format. The proper format is `OR commands -> filename.txt`. This is done using a regular expression inside the `check_OR_input()`. If the command is not properly formatted, then `Error: Redirection command not properly formatted! 'OR commands -> filename.txt' is the proper format` is printed to stderr. 

We then check if the number of tokens passed in is less than 7. OR and -> count as tokens as per the rocketchat message "Seven arguments including PP, OR, ->" from February 6th, 2021. This is done via `OR_tokenize()`.

If the command entered consists of greather than seven tokens, then `Error: Number of arguments cannot exceed 7!` is printed to stderr.

If the number of tokens are okay we get the executable path via `get_exec_path()` and redirect the output via `do_output_redirect()`.

## D

If the first token of the command is PP then we check to see if the input is of the proper format. The proper format is `PP commands -> commands` or `PP commands -> commands -> commands`. This is done using a regular expression inside the `check_PP_input()`. If the command is not properly formatted, then `Error: Piping command not properly formatted! 'PP commands -> commands' is the proper format Note: An additional '-> commands' is supported` is printed to stderr. 

We then check if the number of tokens passed in is less than 7. PP and -> count as tokens as per the rocketchat message "Seven arguments including PP, OR, ->" from February 6th, 2021. This is done via `PP_tokenize()`.

If the command entered consists of greather than seven tokens, then `Error: Number of arguments cannot exceed 7!` is printed to stderr.

If the number of tokens are okay we get the executable path via `get_exec_path()` and pipe the output via `control_piping()`. 
`control_piping()` will choose which piping function to call based on the return value of `PP_tokenize()`.

If there is one arrow (two commands), then `do_one_arrow_pipe()` is called, if there are two arrows (three commands) then `do_two_arrow_pipe()` is called.