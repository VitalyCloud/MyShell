
#include "Headers/ShellCore.h"

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "Headers/Builtin.h"
#include "Headers/ShellParser.h"

char*   shellReadLine(void);
char**  shellSplitLine(char*);
int     shellExecute(char**);
void    parseGetEnv(char*, char*);
int     parseVariableFunc(char *, char*);
void    replaceVariables(char **, int);
int     parseSetEnv(char*);

#define ARGV_LENGHT 64
#define ARGV_SIZE 80

//There's a LOGIN_NAME_MAX constant, accessible via limits.h.
//LOGIN_NAME_MAX was not found in limits.h, only _POSIX_HOST_NAME_MAX
void shellLoop() {
    char *line;
    char **args;
    int status;
    char currentDir[FILENAME_MAX];
    char userName[_POSIX_HOST_NAME_MAX];

    strcpy(userName, getenv("LOGNAME"));
    chdir(getenv("HOME"));
    do {
        getcwd(currentDir, FILENAME_MAX);
        printf("(%s) %s > ",userName, currentDir);
        line = shellReadLine();
        args = shellSplitLine(line);
        status = shellExecute(args);
        
        for(int i=0; i<ARGV_SIZE; i++) {
            free(args[i]);
        }
        free(line);
        free(args);
    } while (status);
}

char* shellReadLine(void) {
    char *line = NULL;
    size_t bufsize = 0; // have getline allocate a buffer for us

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

char** shellSplitLine(char *line) {
    char **output = malloc(ARGV_SIZE * sizeof(char*));
    for(int i=0; i<ARGV_SIZE; i++) {
        output[i] = malloc(ARGV_LENGHT+1);
    }
    size_t argc = split(line, output, 20);
    output[argc] = NULL;
    replaceVariables(output, (int)argc);
    return output;
}

//execvp() won't return unless error, so there's no need for that if.
//We need if(execvp) for error handling via perror.
//For example:
//(fuzrodah) /Users/fuzrodah > qwe
//shell child: No such file or directory
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


int shellExecute(char **args) {
    if(args[0] == NULL)
        return 1;
    //Setinv variable
    if(strstr(args[0], "=") != NULL) {
        if(args[1] != NULL) {
            printf("shell: Expected one argument\n");
            return 1;
        }
        return parseSetEnv(args[0]);
    }
    // ---------------------
    for (int i = 0; i < shell_num_builtins(); i++)
        if(strcmp(args[0], builtin_str[i]) == 0)
            return (*builtin_func[i])(args);
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
    if(setenv(variableName, variableValue, 1) == -1) {
        perror("shell: ");
    }
    return 1;
}

int parseVariableFunc(char* expression, char* buffer) {
    FILE *read_fp;
    int chars_read;
    
    read_fp = popen(expression, "r");
    if(read_fp != NULL) {
        chars_read = (int)fread(buffer, sizeof(char), BUFSIZ, read_fp);
        buffer[chars_read-1] = '\0';
        pclose(read_fp);
        return 1;
    }
    return 0;
}

void removeCharFromString(char *p, char c) {
    if(p==NULL)
        return;
    char *pDest = p;
    while(*p) {
        if(*p != c)
            *pDest++ = *p;
        p++;
    }
    *pDest = '\0';
}

void replaceVariables(char **argv, int size) {
    for(int i=0; i<size; i++) {
        
        char *line = argv[i];
        //Remove " and '
        removeCharFromString(line, '\"');
        removeCharFromString(line, '\'');
        removeCharFromString(line, '\n');
        
        char *start = strstr(line, "$");
        char *end = strchr(line, '\0');
        if(start != NULL) {
            if(*(start+1) == '(') {
                int count = (int)(end-start-3);
                char buffer[ARGV_LENGHT];
                strncpy(buffer, start+2, count);
                buffer[count] = '\0';
                strcpy(line, buffer);
                parseVariableFunc(line, line);
            } else {
                char *env = getenv(start+1);
                if(env!=NULL)
                    strcpy(line, env);
            }
        }
    }
}
