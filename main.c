

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void shellLoop();


int main(int argc, char **argv) {

    shellLoop();

    return EXIT_SUCCESS;
}

char*  shellReadLine();
char**  shellSplitLine(char*);
int shellExecute(char**);

void shellLoop() {
  char *line;
  char **args;
  int status;

  do {
    printf(">");
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
    tokens[position] = token;
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


// BUILTIN Functions ----

int shell_cd(char** args);
int shell_help(char** args);
int shell_exit(char** args);

char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &shell_cd,
  &shell_help,
  &shell_exit
};

int shell_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

//Implementation
int shell_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "shell: expected argument to \"cd\"\n");
  } else {
    if(chdir(args[1]) != 0) {
      perror("shell");
    }
    char currentDir[FILENAME_MAX];
    getcwd(currentDir, FILENAME_MAX);
    printf("%s\n", currentDir);
  }
  return 1;
}

int shell_help(char **args) {
  int i;
  printf("Gleb Grinkevich's Shell\n");
  printf("Type program names and arguments, ant hit enter.\n");
  printf("The following are built in: \n");

  for (i = 0; i < shell_num_builtins(); i++) {
    printf("\t%s\n", builtin_str[i]);
  }

  printf("Use the man command for information n other programs.\n");
  return 1;
}

int shell_exit(char **args) {
  return 0;
}

// ------------------------

int shellExecute(char **args) {
  int i;
  if(args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < shell_num_builtins(); i++) {
    if(strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return shellLaunch(args);
}
