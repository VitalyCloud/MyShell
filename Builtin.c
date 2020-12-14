
#include "Headers/Builtin.h"
#include <unistd.h>

int shell_cd(char**);
int shell_help(char**);
int shell_exit(char**);

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

// ------  Implementation  -----------
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

// -----------------------------------
