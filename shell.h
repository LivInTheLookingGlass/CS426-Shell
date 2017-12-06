#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <wordexp.h>

extern char **environ;
size_t cached_path_index = 0, cached_user_index = 0;
char *prompt = (char *) ">>> ";
char prompt_changed = 0;

char *trimwhitespace(char *str);

char **split_on_char(char *string, const char *tok, size_t *len);
char *get_path();
char *get_user();
char **parse_path(size_t *len);
char *translate_home(char *string, size_t *len);
char run_command(char *command, char silent_dne, char silent_err);
char parse_command(char *command);
void doubleampprocess (char* command);
void semicolonprocess (char* command);
int main(int argc, char **argv);
