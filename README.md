Authors:
    Aarya Dalal
    Yash Desai

Design Notes:

    This is a shell program that has both an interactive and a batch mode.
    It includes built-in commands like cd, pwd, which, exit, and die.
    It also handles conditionals like "and" and "or".
    Handles wildcards('*') and implements command pipelines using execv(), fork(), and pipe().
    Also supports redirection ('<' and '>')

Functions:

     void executeCommands(char *input)

        It parses and executes a single line of input.
        It tokenizes input, handles built-in commands, conditionals (and/or), redirection, pipelines and wildcards.
        It executes external commands via fork() and execv().

    int findPath(char *cmd, char *path)

        It searches predefined directories: /usr/local/bin, /usr/bin, and /bin.

    void pipeCommand(char *input)

        It executes commands separated by a single pipeline ('|').
        It uses pipe(), fork(), dup2()
        It redirects output of the first command to the input of the second command.

    int wildcard(char *common, char **results, int totalResults)

        It matches filenames with specified prefix and suffix patterns.
        It returns an array of matching filenames or the original token if no match found.

    char* trim(char *str)

        It removes leading and trailing whitespace from strings.

    

    char** getArgs(char* command)
    
        Gets the arguments from the terminal

Testing:
    
    Our test.sh ensures all functionalities implemented within the shell work accordingly. 
    We made a file to test all the commands at once with errors and also tested it by ourselves in batch and interative mode.
    Key areas of focus include:
        1. Basic shell functionality (pwd, echo, cd, which)
        2. Error handling for invalid commands, arguments, and paths
        3. Input/output redirection
        4. Conditional logic (and, or)
        5. Wildcards (*.txt, *.c)
        6. Pipelines
        7. Handles comments in commands
