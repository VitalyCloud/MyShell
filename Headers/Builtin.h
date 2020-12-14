
#ifndef Builtin_h
#define Builtin_h

#include <stdio.h>

extern char *builtin_str[];
extern int (*builtin_func[]) (char **);
int shell_num_builtins(void);

#endif /* Builtin_h */
