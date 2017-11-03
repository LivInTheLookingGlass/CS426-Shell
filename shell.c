/*
Points done:

  1 Can run an executable
		/bin/ls
	1 You search the path for the executable
		ls
  1 Knows how to change directory
		cd /fred
  1 Replace "~" with the home directory
		rm ~/junkfile
  1 Control-L clears the screen
		ctrl-l = clear screen
  2 Change Prompt
    PS1="what is you command?"

Total: 7/15 G
       0/15 D
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

extern char **environ;
size_t cached_path_index = 0, cached_user_index = 0;
char *prompt = (char *) ">>> ";
char prompt_changed = 0;

char **split_on_char(char *string, char tok, size_t *len) {
    char **ret = (char **) malloc(sizeof(char **) * 100),
          *token = NULL;
    *len = 0;
    // loop idea gotten from https://stackoverflow.com/a/26228023
    while ((token = strsep(&string, &tok))) {
        ret[(*len)++] = token;
        if ((*len) > 100)   {
            ret = (char **) realloc(ret, sizeof(char **) * (*len + 1));
        }
    }
    return (char **) realloc(ret, sizeof(char **) * (*len));
}

char *get_path()    {
    if (!strncmp("PATH=", environ[cached_path_index], 5))   {
        return environ[cached_path_index] + 5;
    }
    for (size_t i = 0; environ[i] != NULL; i++) {
        if (!strncmp("PATH=", environ[i], 5))   {
            cached_path_index = i;
            return environ[i] + 5;
        }
    }
    return NULL;
}

char *get_user()  {
    if (!strncmp("USER=", environ[cached_user_index], 5))   {
        return environ[cached_user_index] + 5;
    }
    for (size_t i = 0; environ[i] != NULL; i++) {
        if (!strncmp("USER=", environ[i], 5))   {
            cached_user_index = i;
            return environ[i] + 5;
        }
    }
    return NULL;
}

char **parse_path(size_t *len) {
    char *PATH = (char *) malloc(sizeof(char) * (strlen(get_path()) + 1));
    strcpy(PATH, get_path());
    return split_on_char(PATH, ':', len);
}

char *translate_home(char *string, size_t *len) {
    if (string[0] == '~')  {
        *len = 5 + strlen(get_user()) + strlen(string);
        char *new_path = (char *) malloc(sizeof(char) * (*len));
        memcpy(new_path, "/home/", 6);
        strcpy(new_path + 6, get_user());
        strcpy(new_path + 6 + strlen(get_user()), string + 1);
        return new_path;
    }
    *len = -1;
    return string;
}

char run_command(char *command, char silent_dne) {
    size_t len = 0,
          *lens;
    char **parsed = split_on_char(command, ' ', &len);
    char ret = 0;
    lens = (size_t *) malloc(sizeof(size_t) * len);
    for (size_t i = 0; i < len; i++)  {
        parsed[i] = translate_home(parsed[i], lens + i);
    }
    pid_t pid = fork();
    if (pid == 0) {
        execvp(parsed[0], parsed);
        exit(-1);
    }
    else    {
        int status;
        waitpid(pid, &status, WUNTRACED);
        if (WEXITSTATUS(status))  {
            ret = WEXITSTATUS(status);
            if (ret == -1)  {
                if (!silent_dne)
                    printf("%s does not exist... (╯°□°）╯︵ ┻━┻\n", parsed[0]);
            }
            else if (ret) {
                printf("%s failed! (Status %i)... ಠ_ಠ\n", parsed[0], ret);
            }
        }
    }
    for (size_t i = 0; i < len; i++)  {
        if (lens[i] != (size_t) -1)
           free(parsed[i]);
    }
    free(parsed);
    free(lens);
    return ret;
}

int main(int argc, char **argv) {
    if (get_path() != NULL) {
        printf("PATH is %s\n", get_path());
        size_t len;
        char **parsed_path = parse_path(&len);
        if (get_user() != NULL) {
            size_t user_len = strlen(get_user());
            prompt = (char *) calloc(sizeof(char), user_len + 15);
            strncpy(prompt, get_user(), user_len);
            strcpy(prompt + user_len, " says what? | ");
            prompt_changed = 1;
        }
        while (1) {
            char *command = readline(prompt);
            if (!strlen(command)) {}
            else if (!strncmp("exit()", command, 6)) {
                exit(0);
            }
            else if (!strncmp("exit", command, 4))  {
                printf("Use exit() to exit 凸ಠ益ಠ)凸\n");
            }
            else if (!strncmp("PS1=\"", command, 5))  {
                command += 5;
                size_t pos = 0;
                while(command[pos] != '"')
                    ++pos;
                if (prompt_changed)
                    free(prompt);
                prompt = (char *) calloc(sizeof(char), pos + 1);
                strncpy(prompt, command, pos);
                prompt_changed = 1;
            }
            else if (!strncmp("help", command, 4)) {
                printf("Just use it like bash! Gosh! ｏ( ><)o\n");
            }
            else if (!strncmp("cd", command, 2))  {
                size_t len = 0;
                char *translated = translate_home(command + 3, &len);
                printf("%s\n", translated);
                chdir(translated);
                if (len == (size_t) -1) {
                    free(translated);
                }
            }
            else if (!strncmp("/", command, 1) ||
                     !strncmp(".", command, 1) ||
                     !strncmp("~", command, 1)) {
                run_command(command, 0);
            }
            else  {
                size_t command_len = strlen(command);
                char worked = 0;
                for (size_t i = 0; i < len; i++)    {
                    // printf("PATH element %0lu is %s\n", i+1,  parsed_path[i]);
                    size_t segment_len = strlen(parsed_path[i]);
                    char *program = (char *) malloc(sizeof(char) * (segment_len + command_len + 1));
                    memcpy(program, parsed_path[i], segment_len);
                    program[segment_len] = '/';
                    strcpy(program + segment_len + 1, command);
                    int status = run_command(program, 1);
                    free(program);
                    // printf("%i\n", status);
                    if (!status || status != -1)  {
                        worked = !status;
                        break;
                    }
                }
                if (!worked)  {
                    size_t _;
                    char **parsed = split_on_char(command, ' ', &_);
                    printf("%s does not exits... ಠ_ಠ\n", parsed[0]);
                    free(parsed);
                }
            }
        }
    }
}
