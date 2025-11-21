#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>

#define BUFFER_SIZE 4096
#define MAX_ARGS 512
#define MAX_TOKENS 1024
#define MAX_STR 1024

// Global variables for conditional logic
int lastStatus = 0;
int totalCommands = 0;

// Function declarations
void executeCommands(char *input);
int findPath(char *cmd, char *path);
void pipeCommand(char *input);
int wildcard(char *common, char **results, int totalResults);
char *trim(char *str);
void getArgs(char *command, char **fArgs);

int main(int argc, char *argv[])
{
    char command[BUFFER_SIZE];

    // Interactive Mode
    if (argc == 1 && isatty(fileno(stdin)))
    {
        printf("Welcome to my shell!\n");
        while (1)
        {
            printf("mysh> ");
            fflush(stdout);
            int bytes = read(STDIN_FILENO, command, BUFFER_SIZE - 1);
            if (bytes <= 0)
            {
                printf("mysh: exiting\n");
                break;
            }
            command[bytes] = '\0';
            command[strcspn(command, "\n")] = '\0';
            executeCommands(command);
        }
    }
    // Batch Mode or piped input
    else
    {
        if (argc > 1)
        {
            FILE *f = fopen(argv[1], "r");
            if (!f)
            {
                perror("Error opening file");
                return EXIT_FAILURE;
            }
            while (fgets(command, BUFFER_SIZE, f))
            {
                command[strcspn(command, "\n")] = '\0';
                executeCommands(command);
            }
            fclose(f);
        }
        else
        {
            char *line = NULL;
            int len = 0;
            while (getline(&line, &len, stdin) != -1)
            {
                line[strcspn(line, "\n")] = '\0';
                executeCommands(line);
            }
            free(line);
        }
    }
    return 0;
}

// executeCommands: tokenizes the input, handles conditionals and redirections,
// manually expands wildcards in tokens that contain "*", then executes the command.
void executeCommands(char *input)
{
    if (input[0] == '#')
        return;

    // Handle pipeline if present.
    if (strchr(input, '|') != NULL)
    {
        pipeCommand(input);
        totalCommands++;
        return;
    }

    // Tokenize the input by spaces.
    char *tokens[MAX_ARGS];
    int i = 0;
    char *tok = strtok(input, " ");
    while (tok && i < MAX_ARGS - 1)
    {
        tokens[i++] = tok;
        tok = strtok(NULL, " ");
    }
    tokens[i] = NULL;
    if (i == 0)
        return;

    // Prevent conditionals as the first command.
    if ((strcmp(tokens[0], "and") == 0 || strcmp(tokens[0], "or") == 0) && totalCommands == 0)
    {
        fprintf(stderr, "Error: conditional commands cannot be the first command\n");
        return;
    }

    // Process conditionals.
    if (strcmp(tokens[0], "and") == 0)
    {
        if (lastStatus != 0)
            return; // Skip if previous command failed.
        for (int j = 0; j < i - 1; j++)
            tokens[j] = tokens[j + 1];
        tokens[i - 1] = NULL;
        i--;
    }
    else if (strcmp(tokens[0], "or") == 0)
    {
        if (lastStatus == 0)
            return; // Skip if previous command succeeded.
        for (int j = 0; j < i - 1; j++)
            tokens[j] = tokens[j + 1];
        tokens[i - 1] = NULL;
        i--;
    }

    // Process redirection tokens.
    char *inputFile = NULL, *outputFile = NULL;
    char *cleanArgs[MAX_ARGS];
    int j = 0, k = 0;
    while (j < i)
    {
        if (strcmp(tokens[j], "<") == 0)
        {
            if (tokens[j + 1] == NULL)
            {
                fprintf(stderr, "Error: no input file specified\n");
                lastStatus = 1;
                return;
            }
            inputFile = tokens[j + 1];
            j += 2;
        }
        else if (strcmp(tokens[j], ">") == 0)
        {
            if (tokens[j + 1] == NULL)
            {
                fprintf(stderr, "Error: no output file specified\n");
                lastStatus = 1;
                return;
            }
            outputFile = tokens[j + 1];
            j += 2;
        }
        else
        {
            cleanArgs[k++] = tokens[j++];
        }
    }
    cleanArgs[k] = NULL;
    if (k == 0)
        return;

    // Build finalArgs by expanding wildcards manually.
    char *finalArgs[MAX_ARGS];
    int finalCount = 0;
    for (int a = 0; a < k; a++)
    {
        if (strchr(cleanArgs[a], '*') != NULL)
        {
            // Expand token.
            char *matches[MAX_ARGS]; // temporary array for matches
            int count = wildcard(cleanArgs[a], matches, MAX_ARGS);
            for (int m = 0; m < count && finalCount < MAX_ARGS - 1; m++)
            {
                finalArgs[finalCount++] = matches[m];
            }
        }
        else
        {
            finalArgs[finalCount++] = cleanArgs[a];
        }
    }
    finalArgs[finalCount] = NULL;
    if (finalArgs[0] == NULL)
        return;

    // Handle built-in commands.
    if (strcmp(finalArgs[0], "exit") == 0)
    {
        printf("mysh: exiting\n");
        exit(0);
    }
    if (strcmp(finalArgs[0], "die") == 0)
    {
        for (int a = 1; a < finalCount; a++)
            fprintf(stderr, "%s ", finalArgs[a]);
        fprintf(stderr, "\nmysh: terminating with failure\n");
        lastStatus = 1;
        exit(1);
    }
    if (strcmp(finalArgs[0], "cd") == 0)
    {
        if (finalCount != 2)
        {
            fprintf(stderr, "cd: expects one argument\n");
            lastStatus = 1;
        }
        else if (chdir(finalArgs[1]) != 0)
        {
            perror("cd");
            lastStatus = 1;
        }
        else
        {
            lastStatus = 0;
        }
        totalCommands++;
        return;
    }
    if (strcmp(finalArgs[0], "pwd") == 0)
    {
        char cwd[MAX_STR];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s\n", cwd);
            lastStatus = 0;
        }
        else
        {
            perror("pwd");
            lastStatus = 1;
        }
        totalCommands++;
        return;
    }
    if (strcmp(finalArgs[0], "which") == 0)
    {
        if (finalCount != 2)
        {
            fprintf(stderr, "which: expects one argument\n");
            lastStatus = 1;
        }
        else if (strcmp(finalArgs[1], "cd") == 0 ||
                 strcmp(finalArgs[1], "pwd") == 0 ||
                 strcmp(finalArgs[1], "which") == 0 ||
                 strcmp(finalArgs[1], "exit") == 0 ||
                 strcmp(finalArgs[1], "die") == 0)
        {
            fprintf(stderr, "%s: shell built-in\n", finalArgs[1]);
            lastStatus = 0;
        }
        else
        {
            char path[MAX_STR];
            if (findPath(finalArgs[1], path) == 0)
            {
                printf("%s\n", path);
                lastStatus = 0;
            }
            else
            {
                fprintf(stderr, "%s: not found\n", finalArgs[1]);
                lastStatus = 1;
            }
        }
        totalCommands++;
        return;
    }

    // Execute external command via fork/execv.
    pid_t pid = fork();
    if (pid == 0)
    {
        if (inputFile)
        {
            int fd_in = open(inputFile, O_RDONLY);
            if (fd_in < 0)
            {
                perror("open inputFile");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        if (outputFile)
        {
            int fd_out = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
            if (fd_out < 0)
            {
                perror("open outputFile");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        if (!strchr(finalArgs[0], '/'))
        {
            char path[MAX_STR];
            if (findPath(finalArgs[0], path) != 0)
            {
                fprintf(stderr, "Command not found: %s\n", finalArgs[0]);
                exit(EXIT_FAILURE);
            }
            finalArgs[0] = path;
        }
        execv(finalArgs[0], finalArgs);
        perror("execv");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        int status1;
        waitpid(pid, &status1, 0);
        lastStatus = WEXITSTATUS(status1);
        totalCommands++;
    }
    else
    {
        perror("fork");
        lastStatus = 1;
    }
}

// findPath: Searches for the command in /usr/local/bin, /usr/bin, and /bin.
int findPath(char *cmd, char *path)
{
    char *dirs[] = {"/usr/local/bin", "/usr/bin", "/bin"};
    for (int i = 0; i < 3; i++)
    {
        snprintf(path, MAX_STR, "%s/%s", dirs[i], cmd);
        if (access(path, X_OK) == 0)
            return 0;
    }
    return -1;
}

// pipeCommand: Handles a single pipeline (two commands separated by '|').
void pipeCommand(char *input)
{
    char *cmd1 = trim(strtok(input, "|"));
    char *cmd2 = trim(strtok(NULL, "|"));

    if (!cmd1 || !cmd2)
    {
        fprintf(stderr, "Error: Pipe requires two commands\n");
        lastStatus = 1;
        return;
    }

    int cmdpipe[2];
    if (pipe(cmdpipe) == -1)
    {
        perror("pipe");
        lastStatus = 1;
        return;
    }

    pid_t fProc = fork();
    if (fProc == -1)
    {
        perror("fork 1");
        lastStatus = 1;
        return;
    }
    else if (fProc == 0)
    {
        char *args[MAX_ARGS];
        char binPath[1024];
        getArgs(cmd1, args);
        if (findPath(args[0], binPath)) {
            printf("%s is not a command.\n", args[0]);
            exit(-1);
        }
        dup2(cmdpipe[1], STDOUT_FILENO);
        close(cmdpipe[0]);
        close(cmdpipe[1]);
        execv(binPath, args);
    }

    pid_t sProc = fork();
    if (sProc == -1)
    {
        perror("fork 2");
        lastStatus = 2;
        return;
    }
    else if (sProc == 0)
    {
        char *args[MAX_ARGS];
        char binPath[1024];
        getArgs(cmd2, args);
        if (findPath(args[0], binPath)) {
            printf("%s is not a command.\n", args[0]);
            exit(-1);
        }
        dup2(cmdpipe[0], STDIN_FILENO);
        close(cmdpipe[0]);
        close(cmdpipe[1]);
        execv(binPath, args);
    }

    close(cmdpipe[0]);
    close(cmdpipe[1]);

    int status = 0;

    waitpid(fProc, &status, 0);
    if (WEXITSTATUS(status) != 0)
    {
        printf("Failed to run the first command in the pipeline: %d\n", WEXITSTATUS(status));
        kill(sProc, SIGKILL);
        waitpid(sProc, NULL, 0);
        lastStatus = 1;
        return;
    }

    waitpid(sProc, &status, 0);
    if (WEXITSTATUS(status) != 0)
    {
        printf("Failed to run the second command in the pipeline: %d\n", WEXITSTATUS(status));
        lastStatus = 2;
        return;
    }

    lastStatus = 0;
    totalCommands++;
    return;
}

// wildcard: Manually expands a token containing '*'.
// It splits the token into a directory (if any) and a base common,
// then separates the base common into prefix and suffix parts. The
// directory is then scanned for files whose names start with the prefix
// and end with the suffix. If no matches are found, the original token is returned.
int wildcard(char *common, char **results, int totalResults)
{
    // Check for '*' in common.
    if (strchr(common, '*') == NULL)
    {
        results[0] = strdup(common);
        return 1;
    }

    char directory[MAX_STR] = "";
    char basecommon[MAX_STR] = "";
    char *slash = strrchr(common, '/');
    if (slash)
    {
        int dirLen = slash - common;
        if (dirLen >= MAX_STR)
            dirLen = MAX_STR - 1;
        strncpy(directory, common, dirLen);
        directory[dirLen] = '\0';
        strncpy(basecommon, slash + 1, MAX_STR - 1);
        basecommon[MAX_STR - 1] = '\0';
    }
    else
    {
        strcpy(directory, ".");
        strncpy(basecommon, common, MAX_STR - 1);
        basecommon[MAX_STR - 1] = '\0';
    }

    // Compute the location of '*' in basecommon (not common).
    char *ast = strchr(basecommon, '*');
    if (!ast)
    {
        results[0] = strdup(common);
        return 1;
    }
    int preLen = ast - basecommon;
    char prefix[MAX_STR] = "";
    char suffix[MAX_STR] = "";
    if (preLen < MAX_STR)
    {
        strncpy(prefix, basecommon, preLen);
        prefix[preLen] = '\0';
    }
    strncpy(suffix, ast + 1, MAX_STR - 1);
    suffix[MAX_STR - 1] = '\0';

    DIR *dirp = opendir(directory);
    if (!dirp)
    {
        results[0] = strdup(common);
        return 1;
    }
    int count = 0;
    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL && count < totalResults)
    {
        // Skip hidden files.
        if (dp->d_name[0] == '.')
            continue;
        // Check that name starts with prefix.
        if (strncmp(dp->d_name, prefix, strlen(prefix)) != 0)
            continue;
        int nameLen = strlen(dp->d_name);
        int sufLen = strlen(suffix);
        if (nameLen < sufLen)
            continue;
        if (sufLen > 0)
        {
            if (strcmp(dp->d_name + (nameLen - sufLen), suffix) != 0)
                continue;
        }
        // Build full path if directory was specified.
        char fullPath[MAX_STR] = "";
        if (slash)
            snprintf(fullPath, MAX_STR, "%s/%s", directory, dp->d_name);
        else
            snprintf(fullPath, MAX_STR, "%s", dp->d_name);
        results[count++] = strdup(fullPath);
    }
    closedir(dirp);
    if (count == 0)
    {
        results[0] = strdup(common);
        count = 1;
    }
    return count;
}

char *trim(char *str)
{
    // Trim leading
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str; // All spaces

    // Trim trailing
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    *(end + 1) = '\0';
    return str;
}

void getArgs(char *command, char **fArgs)
{
    // Tokenize the input by spaces.
    char *tokens[MAX_ARGS];
    int i = 0;
    char *tok = strtok(command, " ");
    while (tok && i < MAX_ARGS - 1)
    {
        tokens[i++] = tok;
        tok = strtok(NULL, " ");
    }
    tokens[i] = NULL;
    if (i == 0)
        return;

    // Prevent conditionals as the first command.
    if ((strcmp(tokens[0], "and") == 0 || strcmp(tokens[0], "or") == 0) && totalCommands == 0)
    {
        fprintf(stderr, "Error: conditional commands cannot be the first command\n");
        return;
    }

    // Process conditionals.
    if (strcmp(tokens[0], "and") == 0)
    {
        if (lastStatus != 0)
            return; // Skip if previous command failed.
        for (int j = 0; j < i - 1; j++)
            tokens[j] = tokens[j + 1];
        tokens[i - 1] = NULL;
        i--;
    }
    else if (strcmp(tokens[0], "or") == 0)
    {
        if (lastStatus == 0)
            return; // Skip if previous command succeeded.
        for (int j = 0; j < i - 1; j++)
            tokens[j] = tokens[j + 1];
        tokens[i - 1] = NULL;
        i--;
    }

    // Process redirection tokens.
    char *inputFile = NULL, *outputFile = NULL;
    char *cleanArgs[MAX_ARGS];
    int j = 0, k = 0;
    while (j < i)
    {
        if (strcmp(tokens[j], "<") == 0)
        {
            if (tokens[j + 1] == NULL)
            {
                fprintf(stderr, "Error: no input file specified\n");
                lastStatus = 1;
                return;
            }
            inputFile = tokens[j + 1];
            j += 2;
        }
        else if (strcmp(tokens[j], ">") == 0)
        {
            if (tokens[j + 1] == NULL)
            {
                fprintf(stderr, "Error: no output file specified\n");
                lastStatus = 1;
                return;
            }
            outputFile = tokens[j + 1];
            j += 2;
        }
        else
        {
            cleanArgs[k++] = tokens[j++];
        }
    }
    cleanArgs[k] = NULL;
    if (k == 0)
        return;

    // Build finalArgs by expanding wildcards manually.
    char *finalArgs[MAX_ARGS];
    int finalCount = 0;
    for (int a = 0; a < k; a++)
    {
        if (strchr(cleanArgs[a], '*') != NULL)
        {
            // Expand token.
            char *matches[MAX_ARGS]; // temporary array for matches
            int count = wildcard(cleanArgs[a], matches, MAX_ARGS);
            for (int m = 0; m < count && finalCount < MAX_ARGS - 1; m++)
            {
                finalArgs[finalCount++] = matches[m];
            }
        }
        else
        {
            finalArgs[finalCount++] = cleanArgs[a];
        }
    }
    finalArgs[finalCount] = NULL;
    if (finalArgs[0] == NULL)
        return;

    for (int i = 0; i < MAX_ARGS; i++)
    {
        fArgs[i] = finalArgs[i];
        if (fArgs[i] == NULL)
        {
            return;
        }
    }

    return;
}
