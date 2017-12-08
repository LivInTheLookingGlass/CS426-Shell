/*
Points done:
  1 Saves and reloads history to a file
  1 Catch Keyboard interrupt
        ctrl+c = does nothing
        top, ctrl+c = back to prompt
  2 Opens myshell on startup
  1 Prompt includes username
  2 Can read commands from file
        . myshell
  1 Can run an executable
		/bin/ls
  1 You search the path for the executable
		ls
  1 Knows how to change directory
		cd ..
  2 History/tab completion
        cd CS426
  1 Replace "~" with the home directory
		ls ~
  1 Control-L clears the screen
		ctrl-l = clear screen
  2 Change Prompt
    PS1="what is your command? "
  2 Expands enviornment variables on the command line
        echo $USER
  2 SemiColons, multiple or single
  1 Parses for double ampersands
  3 Can run commands in the background.
        sleep "10s" & echo "first"; ls && ls -l; echo "last"
  3 Suggests commands that are spelled incorrectly or not installed
        los

Total: 27/30

Parses for semicolons, then double ampersands, then executes each command.
*/

#include "shell.h"

char *trimwhitespace(char *str){ //https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;
  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

char **split_on_char(char *string, const char *tok, size_t *len) {
    /*
    * Returns an array of strings, based on the input string, split by tok.
    * len is modified to have the size of the returned array.
    * Note: If you generate from this, you are responsible for free()ing the
    * results
    */
    char **ret = (char **) malloc(sizeof(char **) * 100),
          *token = NULL;
    *len = 0;
    // loop idea gotten from https://stackoverflow.com/a/26228023
    while ((token = strsep(&string, tok))) {
        ret[(*len)++] = token;
        if ((*len) > 100)   {
            ret = (char **) realloc(ret, sizeof(char **) * (*len + 1));
        }
    }
    ret[*len] = NULL;
    return (char **) realloc(ret, sizeof(char **) * (*len + 1));
}

char *get_path()    {
    /*
    * Fetches the path from the environment, caching its position so that it is
    * faster to get the next time. You do not need to free() the result.
    */
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
    /*
    * Fetches the username from the environment, caching its position so that it
    * is faster to get the next time. You do not need to free() the result.
    */
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
    /*
    * Parses the path into an array of strings. Because this calls
    * split_on_char() in the background, you should follow its free() policy
    */
    char *PATH = (char *) malloc(sizeof(char) * (strlen(get_path()) + 1));
    strcpy(PATH, get_path());
    return split_on_char(PATH, ":", len);
}

char *translate_home(char *string, size_t *len) {
    /*
    * Takes a string, and if it begins with a ~, translate that into the
    * appropriate home directory. You are responsible for free()ing the result.
    */
    wordexp_t expanded;
    wordexp(string, &expanded, WRDE_NOCMD);
    *len = strlen(string);
    string = (char *) malloc(sizeof(char) * (*len + 1));
    strcpy(string, expanded.we_wordv[0]);
    wordfree(&expanded);
    return string;
}

char run_command(char *command, char silent_dne, char silent_err, pipe_t pipecommand, pipe_t pipecloses) {
    size_t len = 0,
          *lens;
    char **parsed = split_on_char(command, " ", &len);
    char ret = 0;
    lens = (size_t *) malloc(sizeof(size_t) * len);
    for (size_t i = 0; i < len; i++)  {
        parsed[i] = translate_home(parsed[i], lens + i);
    }

    pid_t pid = fork();
    if (pid == 0) {
        for (int i = 0; i <= 1; i++)    {
            if (pipecloses[i] != i) {
                close(pipecloses[i]);
            }
            if (pipecommand[i] != i)    {
                if (dup2(i, pipecommand[i]) == -1)    {
                    perror("dup2 failed");
                    exit(-1);
                }
            }
        }
        execvp(parsed[0], parsed);
        exit(-1);
    }
    else    {
        int status;
        for (int i = 0; i <= 1; i++)    {
            if (pipecommand[i] != i) {
                close(pipecommand[i]);
            }
        }
        waitpid(pid, &status, WUNTRACED);
        if (WEXITSTATUS(status))  {
            ret = WEXITSTATUS(status);
            if (ret == -1)  {
                if (!silent_dne)
                    printf("%s does not exist... (╯°□°）╯︵ ┻━┻\n", parsed[0]);
            }
            else if (ret) {
                if(!silent_err)
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

char parse_command(char *command, pipe_t pipecommand, pipe_t pipecloses) {
    /*
    * Parses a command and takes the appropriate action. Atm that means:
    * 1. exits
    * 2. complains about exit
    * 3. changes the prompt
    * 4. prints a sarcastic help
    * 5. changes a directory
    * 6. runs a binary
    *
    * Returns: The exit status of the command (0 is good)
    */
    //printf("%s\n", command); Used to verify parse was receving proper command.

    if (!strncmp("exit()", command, 6)) {
        exit(0);
    }
    else if (!strncmp("exit", command, 4))  {
        printf("Use exit() to exit 凸ಠ益ಠ)凸\n");
        return -1;
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
        return 0;
    }
    else if (!strncmp("help", command, 4)) {
        printf("Just use it like bash! Gosh! ｏ( ><)o\n");
        return 0;
    }
    else if (!strncmp("cd", command, 2))  {
        size_t len = 0;
        char *translated = translate_home(command + 3, &len);
        //printf("%s\n", translated);
        char ret = chdir(translated);
        if (ret)
            printf("That directory does not exist\n");
        if (len == (size_t) -1) {
            free(translated);
        }
        return ret;
    }
    else if (!strncmp(". ", command, 2)) {  //wtflolkillmenowz
        size_t len;
        ssize_t read = 0;
        char *nextline = NULL;
        char *filename = translate_home(command + 2, &len);
        FILE *f = fopen(filename, "r");
        free(filename);
        if(f == NULL){
            printf("%s does not exist \n", nextline);
            return 1;
        }
        while ((read = getline(&nextline, &len, f)) != -1)  {
            nextline = trimwhitespace(nextline);
            if (strlen(nextline) != 0)
                semicolonprocess(nextline);
        }
        fclose(f);
        free(nextline);
        return 0;
    }
    else if (!strncmp("/", command, 1) ||
             !strncmp("./", command, 2) ||
             !strncmp("~/", command, 2)) {
        return run_command(command, 0, 0, pipecommand, pipecloses);
    }
    else  {
        size_t len, command_len = strlen(command);
        char worked = 0,
             **parsed_path = parse_path(&len);
        for (size_t i = 0; i < len; i++)    {
            // printf("PATH element %0lu is %s\n", i+1,  parsed_path[i]);
            size_t segment_len = strlen(parsed_path[i]);
            char *program = (char *) malloc(sizeof(char) * (segment_len + command_len + 1));
            memcpy(program, parsed_path[i], segment_len);
            program[segment_len] = '/';
            strcpy(program + segment_len + 1, command);
            int status = run_command(program, 1, 0, pipecommand, pipecloses);
            free(program);
            // printf("%i\n", status);
            if (!status || status != -1)  {
                worked = !status;
                break;
            }
        }
        if (!worked)  {
            size_t _;
            char **parsed = split_on_char(command, " ", &_);
            char *notfound = (char *) "/usr/lib/command-not-found ";
            char *cmd = (char *) malloc(strlen(parsed[0]) + 27 + 1);
            strcpy(cmd, notfound);
            strcpy(cmd + 27, parsed[0]);
            run_command(cmd, 1, 1, pipecommand, pipecloses);
            //printf("%s does not exist... ಠ_ಠ\n", parsed[0]);
            free(parsed);
            free(cmd);
        }
        return !worked;
    }
}

char singleampprocess (char* command){ //Processes commands with doubleamps
    size_t singleamplen = 0;
    char retval = 0;
    pid_t pid;
    char **singleampparsed = split_on_char(command, "&", &singleamplen);
    for(size_t i = 0; i < singleamplen; i++){
        singleampparsed[i] = trimwhitespace(singleampparsed[i]);
        if(strlen(singleampparsed[i])){//If the command is not empty
            if (i == singleamplen - 1 || (pid = fork()) == 0) {// if I'm last or a kid
                retval = pipeprocess(singleampparsed[i]);
                if (!pid)
                    exit(0);
            }
        }
    }
    return retval;
}

void doubleampprocess (char* command){ //Processes commands with doubleamps
    size_t doubleamplen = 0;
    char **doubleampparsed = split_on_char(command, "&&", &doubleamplen);
    for(size_t i = 0; i < doubleamplen; i++){
        doubleampparsed[i] = trimwhitespace(doubleampparsed[i]);
        if(strlen(doubleampparsed[i])){//If the command is not empty
            if(singleampprocess(doubleampparsed[i]))
                break;
        }
    }
}

void semicolonprocess (char* command){ //Processes commands with semicolons, then passes that to double amp for processing.
    size_t semicolonlen = 0;
    char **semicolonparsed = split_on_char(command, ";", &semicolonlen);
    for(size_t i = 0; i < semicolonlen; i++){
        semicolonparsed[i] = trimwhitespace(semicolonparsed[i]);
        if(strlen(semicolonparsed[i])){//If the command is not empty
            singleampprocess(semicolonparsed[i]);
        }
    }
}


char pipeprocess (char * command){
    //Look for pipes, and process data properly
    size_t pipelen = 0;
    char retval = 0;
    char **pipeparsed = split_on_char(command, "|", &pipelen);
    pipe_t pipes[pipelen];
    pipes[0][0] = 0;
    pipes[pipelen-1][1] = 1;

    for(size_t i = 0; i < pipelen; i++){
        pipe_t closes = {0,1};
        if (i < pipelen-1)  {
            pipe_t pipei;
            pipe2(pipei, O_NONBLOCK);//Creates pipe on each command
            pipes[i][1] = pipei[0];//Takes command i output, and sets to current input
            pipes[i+1][0] = pipei[1];//Sets current output to next input.
            closes[1] = pipei[1];
        }
        if (i > 0)  {
            closes[0] = pipes[i-1][1];
        }

        pipeparsed[i] = trimwhitespace(pipeparsed[i]);
        if(strlen(pipeparsed[i])){//If the command is not empty
            retval = parse_command(pipeparsed[i], pipes[i], closes);
        }
    }
    return retval;
}

int main(int argc, char **argv) {
    signal(SIGINT, SIG_IGN);
    using_history();
    read_history(".history");
    if (get_path() != NULL) {
        char startup[10];
        strcpy(startup, ". myshell");
        semicolonprocess(startup); // this is REALLY fragile
        if (get_user() != NULL) {
            size_t user_len = strlen(get_user());
            prompt = (char *) calloc(sizeof(char), user_len + 15);
            strncpy(prompt, get_user(), user_len);
            strcpy(prompt + user_len, " says what? >> ");
            prompt_changed = 1;
        }
        while (1) {
            char *command = readline(prompt);
            if (!strlen(command)) {
                continue;
            }
            else  {
                add_history(command);
                append_history(1, ".history");
                semicolonprocess (command);
            }
        }
    }
}
