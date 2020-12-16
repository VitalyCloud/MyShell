
#include "Headers/ShellCore.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "Headers/Builtin.h"

char*   shellReadLine(void);
char**  shellSplitLine(char*);
int     shellExecute(char**);

void shellLoop() {
    char *line;
    char **args;
    int status;
    char currentDir[FILENAME_MAX];
    char userName[80];
    
    strcpy(userName, getenv("LOGNAME"));
    chdir(getenv("HOME"));
    do {
        getcwd(currentDir, FILENAME_MAX);
        printf("(%s) %s > ",userName, currentDir);
        line = shellReadLine();
        args = shellSplitLine(line);
        status = shellExecute(args);

        free(line);
        free(args);
    } while (status);
}



char* shellReadLine(void) {
    char *line = NULL;
    ssize_t bufsize = 0; // have getline allocate a buffer for us

    if (getline(&line, &bufsize, stdin) == -1){
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);  // We recieved an EOF
        } else  {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

void parseGetEnv(char*, char*);

#define SHELL_TOK_BUFFERSIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"
char** shellSplitLine(char *line) {
    int bufferSize = SHELL_TOK_BUFFERSIZE;
    int position = 0;
    char **tokens = malloc(bufferSize * sizeof(char*));
    char *token;

    if(!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, SHELL_TOK_DELIM);
    while(token != NULL) {
        
        if(token[0] == '$' && token[1] != '\0') {
            tokens[position] = getenv(token+1);
        } else {
            tokens[position] = token;
        }
        
        position++;

        if(position >= bufferSize) {
            bufferSize += SHELL_TOK_BUFFERSIZE;
            tokens = realloc(tokens, bufferSize * sizeof(char*));
            if(!tokens) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, SHELL_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int shellLaunch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if(pid == 0) {
        //Child process
        if(execvp(args[0], args) == -1) {
            perror("shell child");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        //Error forking
        perror("shell");
    } else {
        //Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int parseSetEnv(char*);

int shellExecute(char **args) {
    if(args[0] == NULL) {
        return 1;
    }
    
    //Setinv variable
    if(strstr(args[0], "=") != NULL) {
        if(args[1] != NULL) {
            printf("shell: Expected one argument\n");
            return 1;
        }
        return parseSetEnv(args[0]);
    }
    // ---------------------

    for (int i = 0; i < shell_num_builtins(); i++) {
        if(strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return shellLaunch(args);
}

int parseSetEnv(char* expresison) {
    char* delim = "=";
    
    char* variableName = strtok(expresison, delim);
    char* variableValue = strtok(NULL, delim);
    
    if(strstr(expresison, "=") - expresison == 0 ) {
        printf("shell: Variable name must be specified\n");
        return 1;
    }
    if(!variableValue) {
        variableValue = "";
    }
    
//    printf("Variable: %s\nValue: %s\n", variableName, variableValue);
    if(setenv(variableName, variableValue, 1) == -1) {
        perror("shell: ");
    }

    return 1;
}

