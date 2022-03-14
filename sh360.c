/*
 * sh360.c
 * Derek Robinson
 * V00875105
 * CSC 360, Spring 2021
 */

/*LIBRARIES*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <regex.h>

/*DEFINES*/
#define MAX_NUM_ARGS 7 //Max number of arguments in any command
#define MAX_LINE_LEN 80 //Max number of characters for any line of user input
#define MAX_PROMPT_LEN 10  //Max number of characters in any prompt
#define MAX_DIR_IN_PATH 10 //Max number of directories in any path
#define MAX_NUM_PP_ARROWS 2 //Max number of -> in any piping command

/*GLOBALS*/ 
char filename[MAX_LINE_LEN]; //filename for ouput redirection
// command arguments for piping
char *cmd_head[MAX_NUM_ARGS];
char *cmd_mid[MAX_NUM_ARGS];
char *cmd_tail[MAX_NUM_ARGS];

/*HELPER FUNCTIONS*/
 
/* Detects if the user input contains OR
* References: https://stackoverflow.com/questions/15098936/simple-way-to-check-if-a-string-contains-another-string-in-c
* @param input_copy a copy of the user input
* @return 1 if input_copy contains OR, 0 otherwise
*/
int detect_OR(char input_copy[MAX_LINE_LEN])
{
        if(strstr(input_copy, "OR") != NULL){ //input contains OR
                return 1;
        } else {
                return 0;
        }
}

/* Detects if the user input contains PP
* References: https://stackoverflow.com/questions/15098936/simple-way-to-check-if-a-string-contains-another-string-in-c
* @param input_copy a copy of the user input
* @return 1 if input_copy contains PP, 0 otherwise
*/
int detect_PP(char input_copy[MAX_LINE_LEN])
{
        if(strstr(input_copy, "PP") != NULL){ //input contains OR
                return 1;
        } else {
                return 0;
        }
}

/* Tokenizes the input string for output_redirection. 
* References: appendix_e.c program written by Mike Zastre for UVic CSC 360 2021
* @param tokens the char array to fill with tokens
* @param input the input array taken from the user prior to calling tokenize()
* @return returns -1 if num_tokens exceeds 7, 0 otherwise
*/
int OR_tokenize(char **tokens, char *input, char filename[MAX_LINE_LEN])
{
        char *t;
        int num_tokens = 0; //total number of tokens in the command including OR and ->

        t = strtok(input, " "); //this will be OR
        t = strtok(NULL, " "); //this will be the first argument
        while(t != NULL) {
                num_tokens++;
                if(num_tokens + 1 > MAX_NUM_ARGS){ // + 1 to account for missing the OR
                        fprintf(stderr,"Error: Number of arguments cannot exceed 7!\n");
                        return -1;
                }
                if(strcmp(t, "->") != 0){ //if t != "->" then its a command or argument
                        tokens[num_tokens] = t;
                } else if(strcmp(t, "->") == 0){ //t == '->' here so we take the next token, save it as filename, and exit the loop
                        t = strtok(NULL, " ");
                        if(t != NULL){
                                strncpy(filename, t, MAX_LINE_LEN);
                                break;
                        }
                }
                t = strtok(NULL, " ");
        }
        tokens[num_tokens] = 0;
        return 0;
}

/* Tokenizes the input string for piping. Fills the cmd_head, cmd_mid, and cmd_tail globals
* References: appendix_e.c program written by Mike Zastre for UVic CSC 360 2021
* @param input the input array taken from the user prior to calling tokenize()
* @return the number of arrows (->) in the pipe, or -1 if too many tokens or arrows are present
*/
int PP_tokenize(char *input)
{
        char *t;
        int num_tokens = 0; //number of tokens for the individual sections of the pipe
        int total_tokens = 1; //total number of tokens inclduing PP and ->, starts at 1 to include PP
        int num_arrows = 0; //number of ->
        t = strtok(input, " "); //this will be PP
        t = strtok(NULL, " "); //this will be the first argument

        while(t != NULL && num_arrows <= MAX_NUM_PP_ARROWS) {
                if(total_tokens > MAX_NUM_ARGS){ // + 1 to account for arrays starting at 0
                        fprintf(stderr,"Error: Number of arguments cannot exceed 7!\n");
                        return -1;
                }
                if(strcmp(t, "->") != 0){ //if t != "->" then its a command or argument
                        total_tokens++;
                        switch (num_arrows){
                                case 0:
                                        cmd_head[num_tokens++] = t;
                                        break;
                                case 1:
                                        cmd_mid[num_tokens++] = t;
                                        break;
                                case 2:
                                        cmd_tail[num_tokens++] = t;
                                        break;
                        }
                } else if(strcmp(t, "->") == 0){
                        total_tokens++;
                        num_arrows++;
                        switch (num_arrows){
                                case 0:
                                        cmd_head[num_tokens] = 0;
                                        num_tokens = 0;
                                        break;
                                case 1:
                                        cmd_mid[num_tokens] = 0;
                                        num_tokens = 0;
                                        break;
                                case 2:
                                        cmd_tail[num_tokens] = 0;
                                        num_tokens = 0;
                                        break;
                        }
                }
                t = strtok(NULL, " ");
        }
        return num_arrows;
}

/* Tokenizes the input string for regular use. 
* References: appendix_e.c program written by Mike Zastre for UVic CSC 360 2021
* @param tokens the char array to fill with tokens
* @param input the input array taken from the user prior to calling tokenize()
*/
int regular_tokenize(char **tokens, char *input)
{
        char *t;
        int num_tokens = 0;
        t = strtok(input, " ");
        while(t != NULL) {
                if(num_tokens + 1 > MAX_NUM_ARGS){ // + 1 to account for arrays starting at 0
                        fprintf(stderr,"Error: Number of arguments cannot exceed 7!\n");
                        return -1;
                }
                tokens[num_tokens++] = t;
                t = strtok(NULL, " ");
        }
        tokens[num_tokens] = 0;
        return 0;
}

/* Checks input_copy to see if it's properly formatted to do output redirection by using regular expressions
* References:  https://www.geeksforgeeks.org/regular-expressions-in-c/
*              https://en.wikibooks.org/wiki/Regular_Expressions/POSIX-Extended_Regular_Expressions
*              man regex
* @param input_copy a copy of the user input to check
* @return 0 if properly formatted, 1 if not
*/
int check_OR_input(char input_copy[MAX_LINE_LEN])
{
        regex_t regex;
        regcomp(&regex,"^(OR)( ){1}(.){1,}( ){1}(->)( ){1}(.){1,}(\\.){1}(.){1,}", REG_EXTENDED);
        int return_value = regexec(&regex, input_copy, 0, NULL, 0);

        if (return_value == 0){
                return 1;
        }else if (return_value == REG_NOMATCH){
                fprintf(stderr, "Error: Redirection command not properly formatted!\n");
                fprintf(stderr, "`OR commands -> filename.txt` is the proper format\n");
                return 0;
        }else{
                fprintf(stderr, "An error occured in compiling a regular expression.\n");
                return -1;
        }

}

/* Checks input_copy to see if it's properly formatted to do piping by using regular expressions
* References:  https://www.geeksforgeeks.org/regular-expressions-in-c/
*              https://en.wikibooks.org/wiki/Regular_Expressions/POSIX-Extended_Regular_Expressions
*              man regex
* @param input_copy a copy of the user input to check
* @return 0 if properly formatted, 1 if not
*/
int check_PP_input(char input_copy[MAX_LINE_LEN])
{
        regex_t regex;
        regcomp(&regex,"^(PP)( ){1}(.){1,}( ){1}(->)( ){1}(.){1,}", REG_EXTENDED);
        int return_value = regexec(&regex, input_copy, 0, NULL, 0);

        if (return_value == 0){
                return 1;
        }else if (return_value == REG_NOMATCH){
                fprintf(stderr, "Error: Piping command not properly formatted!\n");
                fprintf(stderr, "`PP commands -> commands` is the proper format\n");
                fprintf(stderr, "Note: An additional `-> commands` is supported\n");
                return 0;
        }else{
                fprintf(stderr, "An error occured in compiling a regular expression.\n");
                return -1;
        }
}

/* Runs shell commands with the use of execve() and fork(). 
* References: appendix_b.c program written by Mike Zastre from UVic CSC 360 2021
*             http://digi-cron.com:8080/programmations/c/lectures/6-fork.html
* @param args an argument to pass to execve, contains the binary executable for a given command and all its parameters
*/
int run_command(char *args[])
{
        int pid;
        int status;
        char *envp[] = {0};

        switch (pid = fork()) {
                case -1:
                        fprintf(stderr, "fork error\n");
                        exit(1);
                case 0:           /* child */
                        execve(args[0], args, envp);
                        exit(123);
                default:          /* parent */
                        break;
        }

        waitpid(pid, &status, 0);

        return pid;
}

/* Reads .sh360rc file, places first line into prompt and the rest of lines into paths
* References:  https://www.tutorialspoint.com/c_standard_library/c_function_fopen.htm
*              https://www.tutorialspoint.com/c_standard_library/c_function_fgetc.htm
*              https://www.tutorialspoint.com/c_standard_library/c_function_fgets.htm
*              https://www.tutorialspoint.com/c_standard_library/c_function_strncpy.htm
* @return path_num the number of paths in the paths golbal variable
*/
int read_rc(char prompt[10], char paths[MAX_DIR_IN_PATH][MAX_LINE_LEN])
{
        FILE *fptr = fopen(".sh360rc", "r");
        char str[20];
        char c;

        if(fptr == NULL){
                fprintf(stderr, "Error opening .sh360rc\n");
                exit(1);
        }

        /* get the prompt from first line of .sh360rc */
        int prompt_len = 0;
        c = fgetc(fptr);
        while(c != '\n' && prompt_len < MAX_PROMPT_LEN){
                prompt[prompt_len++] = c;
                c = fgetc(fptr);
        }
        prompt[prompt_len] = '\0';

        /* get rest of file */
        int path_num = 0;
        while(fgets(str, 20, fptr) != NULL && path_num < MAX_DIR_IN_PATH){
                if(str[strlen(str) - 1] == '\n'){
                        str[strlen(str) - 1] = '/';
                        str[strlen(str)] = '\0';
                }
                strncpy(paths[path_num++], str, 80);
        }
                
        fclose(fptr);
        return path_num;
}

/* Gets the path of the executable which the user inputs, throws an error if not found.
* References:  https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-concatenation
*              https://codeforwin.org/2018/03/c-program-check-file-or-directory-exists-not.html
* @param full_path the path of the executable which the user wants to run, this is passed in to write to
* @param cmd the command the user wishes to run, gets appended to the proper path
* @param paths all the paths in the .sh360rc file as an array
* @return the string which represents the path to the binary executable of cmd
*/
void get_exec_path(char full_path[30], char cmd[], char paths[MAX_DIR_IN_PATH][MAX_LINE_LEN])
{
        int i;
        for(i = 0; i < MAX_DIR_IN_PATH; i++){
                strncpy(full_path, paths[i], 30);
                strcat(full_path, cmd);
                if(access(full_path, X_OK) == 0){
                        break;
                }
        }
        if(strcmp(full_path, cmd) == 0){ //no binary found in .sh360rc so we cannot run command
                strncpy(full_path, "\0", 1);
                fprintf(stderr, "%s: command not found\n", cmd);
        }
}

/* Does output redirection
* References: appendix_c.c program written by Mike Zastre from UVic CSC 360 2021
*             https://stackoverflow.com/questions/11042218/c-restore-stdout-to-terminal
*
*/
int do_output_redirect(char *args[], char filename[MAX_LINE_LEN])
{
        int fd;
        int stdout_fd;
        int stderr_fd;

        fd = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if (fd == -1) {
            fprintf(stderr, "cannot open %s for writing\n", filename);
            exit(1);
        }
        stdout_fd = dup(1);
        stderr_fd = dup(2);
        dup2(fd, 1);
        dup2(fd, 2);
        run_command(args);
        close(fd);
        dup2(stdout_fd,1);
        dup2(stderr_fd,2);
        return 0;
}

/* Creates a pipe between two commands and executes those commands
* References:  appendix_d.c program written by Mike Zastre from UVic CSC 360 2021     
* @param paths the paths in .sh360rc to pass along to get_exec_path()
*/
void do_one_arrow_pipe(char paths[MAX_DIR_IN_PATH][MAX_LINE_LEN])
{
        char *envp[] = { 0 };
        int status;
        pid_t pid_head, pid_mid;
        int fd[2];
        char full_path[30];

        int pipe_return = pipe(fd);
        if(pipe_return == -1){
                fprintf(stderr, "pipe error\n");
                exit(1);
        }

        pid_head = fork();
        if (pid_head == 0) {
                dup2(fd[1], 1);
                close(fd[0]);
                get_exec_path(full_path, cmd_head[0], paths);
                if(strcmp(full_path, "\0") != 0){
                        cmd_head[0] = full_path;
                        execve(cmd_head[0], cmd_head, envp);
                }
        } else if(pid_head == -1){
                fprintf(stderr, "fork error\n");
                exit(1);
        }
        
        pid_mid = fork();
        if (pid_mid == 0) {
                dup2(fd[0], 0);
                close(fd[1]);
                get_exec_path(full_path, cmd_mid[0], paths);
                if(strcmp(full_path, "\0") != 0){
                        cmd_mid[0] = full_path;
                        execve(cmd_mid[0], cmd_mid, envp);
                }
        } else if(pid_mid == -1){
                fprintf(stderr, "fork error\n");
                exit(1);
        }
        
        close(fd[0]);
        close(fd[1]);
        waitpid(pid_head, &status, 0);
        waitpid(pid_mid, &status, 0);
}

/* Creates a pipe between three commands and executes those commands
* References:  appendix_d.c program written by Mike Zastre from UVic CSC 360 2021
*              https://stackoverflow.com/questions/34010502/c-program-to-perform-a-pipe-on-three-commands
* @param paths the paths in .sh360rc to pass along to get_exec_path()
*/
void do_two_arrow_pipe(char paths[MAX_DIR_IN_PATH][MAX_LINE_LEN])
{

        char *envp[] = { 0 };
        int status;
        pid_t pid_head, pid_mid, pid_tail;
        int fd[2]; // mid -> tail
        int fd1[2]; //head -> mid
        char full_path[30];

        int pipe_return = pipe(fd); //set up pipe for 
        if(pipe_return == -1){
                fprintf(stderr, "pipe error\n");
                exit(1);
        } else {
                pid_tail = fork();
                if (pid_tail == 0) { //in tail
                        dup2(fd[0], 0); //connect fd[0] to stdin
                        close(fd[0]);
                        close(fd[1]);
                        get_exec_path(full_path, cmd_tail[0], paths);
                        if(strcmp(full_path, "\0") != 0){
                                cmd_tail[0] = full_path;
                                execve(cmd_tail[0], cmd_tail, envp);
                        }
                } else if(pid_tail == -1){
                        fprintf(stderr, "fork error\n");
                        exit(1);
                } else { //in parent
                        int pipe_return = pipe(fd1); //set up pipe for 
                        if(pipe_return == -1){
                                fprintf(stderr, "pipe error\n");
                                exit(1);
                        } else {
                                pid_head = fork();
                                if (pid_head == 0) { //in head
                                        dup2(fd1[1], 1);
                                        close(fd1[0]);
                                        close(fd1[1]);
                                        get_exec_path(full_path, cmd_head[0], paths);
                                        if(strcmp(full_path, "\0") != 0){
                                                cmd_head[0] = full_path;
                                                execve(cmd_head[0], cmd_head, envp);
                                        }
                                } else if(pid_tail == -1){
                                        fprintf(stderr, "fork error\n");
                                        exit(1);
                                }

                                pid_mid = fork();
                                if (pid_mid == 0) { //in mid
                                        dup2(fd1[0], 0);
                                        dup2(fd[1], 1);
                                        close(fd1[0]);
                                        close(fd1[1]);
                                        get_exec_path(full_path, cmd_mid[0], paths);
                                        if(strcmp(full_path, "\0") != 0){
                                                cmd_mid[0] = full_path;
                                                execve(cmd_mid[0], cmd_mid, envp);
                                        }
                                } else if(pid_mid == -1){
                                        fprintf(stderr, "fork error\n");
                                        exit(1);
                                }
                        }
                }
        }
        close(fd[0]);
        close(fd[1]);
        close(fd1[0]);
        close(fd1[1]);
        waitpid(pid_head, &status, 0);
        waitpid(pid_mid, &status, 0);
        waitpid(pid_tail, &status, 0);
}

/* Controls which piping function to call
 * @param num_arrrows the number of arrows in the piping command, lets us know which piping function to call
 * @param paths the paths in .sh360rc, simply here just to pass along to the other piping functions
*/
void control_piping(int num_arrows, char paths[MAX_DIR_IN_PATH][MAX_LINE_LEN]){
        
        switch (num_arrows)
        {
                case 1:
                        do_one_arrow_pipe(paths);             
                        break;
                
                case 2:
                        do_two_arrow_pipe(paths);
                        break;
        }
}

/* Controls the main flow of the program
 * References: appenddix_a.c written by Mike Zastre for UVic CSC 360 2021
 */
int main(int argc, char *argv[])
{
        char full_path[30];
        char prompt[MAX_PROMPT_LEN];
        char paths[MAX_DIR_IN_PATH][MAX_LINE_LEN];
        char filename[MAX_LINE_LEN];

        read_rc(prompt, paths);

        for(;;){
                char input[MAX_LINE_LEN];
                char input_copy[MAX_LINE_LEN];
                fprintf(stdout, "%s ", prompt);
                fflush(stdout);
                fgets(input, MAX_LINE_LEN + 2, stdin); //+ 2 for newline and one extra character so we can check length

                if(input[0] != '\n'){ //if input is not a new line
                
                        if(input[strlen(input) - 1] == '\n'){
                                input[strlen(input) - 1] = '\0';
                        }

                        strncpy(input_copy, input, MAX_LINE_LEN); //make a copy of the input

                        if(strcmp(input, "exit") == 0){
                                exit(0);
                        } else if(strcmp(input, "clear") == 0){
                                const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
                                write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
                        }

                        char *tokens[MAX_LINE_LEN];
                        int OR = detect_OR(input_copy);
                        int PP = detect_PP(input_copy);
                                
                        if(OR == 1){ //we have detected you want to do output redirection
                                if(check_OR_input(input_copy) == 1){
                                        int tokens_okay = OR_tokenize(tokens, input, filename);
                                        if(tokens_okay == 0){
                                                get_exec_path(full_path, tokens[0], paths);
                                                if(strcmp(full_path, "\0") != 0){
                                                        tokens[0] = full_path;
                                                        do_output_redirect(tokens, filename);
                                                }
                                        }
                                }
                        } else if(PP == 1){ //we have detected you want to do command piping
                                if(check_PP_input(input_copy) == 1){
                                        int num_arrows = PP_tokenize(input);
                                        control_piping(num_arrows, paths);
                                }
                        } else if(PP == 0 && OR == 0){ //regular commands
                                regular_tokenize(tokens, input);
                                get_exec_path(full_path, tokens[0], paths);
                                if(strcmp(full_path, "\0") != 0){
                                        tokens[0] = full_path;
                                        run_command(tokens);
                                }
                        }
                }           
        }
}