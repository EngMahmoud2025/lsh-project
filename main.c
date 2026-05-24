/***************************************************************************//**
  @file         main.c
  @author       Stephen Brennan / Mahmoud Ragab
  @brief        LSH (LibStephen SHell) - Improved Version
*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char **environ;

#define HISTORY_SIZE 100

char *history[HISTORY_SIZE];
int history_count = 0;

/*
  Function declarations for builtin shell commands
*/

int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

int lsh_pwd(char **args);
int lsh_echo(char **args);
int lsh_history(char **args);
int lsh_env(char **args);

/*
  List of builtin commands
*/

char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "pwd",
  "echo",
  "history",
  "env"
};

int (*builtin_func[]) (char **) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_pwd,
  &lsh_echo,
  &lsh_history,
  &lsh_env
};

int lsh_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

/*
  Builtin implementations
*/

int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }

  return 1;
}

int lsh_help(char **args)
{
  int i;

  printf("Mahmoud's LSH\n");
  printf("Type program names and arguments, then press enter.\n");
  printf("Built in commands are:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");

  return 1;
}

int lsh_exit(char **args)
{
  return 0;
}

/*
  pwd command
*/

int lsh_pwd(char **args)
{
  if (args[1] != NULL) {
    fprintf(stderr, "lsh warning: pwd does not take arguments\n");
  }

  char cwd[1024];

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("lsh");
  }

  return 1;
}

/*
  echo command
*/

int lsh_echo(char **args)
{
  int i = 1;

  while (args[i] != NULL) {

    printf("%s", args[i]);

    if (args[i + 1] != NULL) {
      printf(" ");
    }

    i++;
  }

  printf("\n");

  return 1;
}

/*
  history command
*/

int lsh_history(char **args)
{
  if (args[1] != NULL) {
    fprintf(stderr, "lsh warning: history does not take arguments\n");
  }

  int i;

  for (i = 0; i < history_count; i++) {
    printf("%d %s\n", i + 1, history[i]);
  }

  return 1;
}

/*
  env command
*/

int lsh_env(char **args)
{
  if (args[1] != NULL) {
    fprintf(stderr, "lsh warning: env does not take arguments\n");
  }

  int i = 0;

  while (environ[i] != NULL) {
    printf("%s\n", environ[i]);
    i++;
  }

  return 1;
}

/*
  Add command to history
*/

void add_to_history(char *line)
{
  if (line == NULL || line[0] == '\0') {
    return;
  }

  char *line_copy = malloc(strlen(line) + 1);

  if (!line_copy) {
    fprintf(stderr, "lsh: allocation error\n");
    return;
  }

  strcpy(line_copy, line);

  if (history_count < HISTORY_SIZE) {

    history[history_count] = line_copy;
    history_count++;

  } else {

    free(history[0]);

    for (int i = 1; i < HISTORY_SIZE; i++) {
      history[i - 1] = history[i];
    }

    history[HISTORY_SIZE - 1] = line_copy;
  }
}

/*
  Launch external command
*/

int lsh_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();

  if (pid == 0) {

    // Child process

    if (execvp(args[0], args) == -1) {
      perror("lsh");
    }

    exit(EXIT_FAILURE);

  } else if (pid < 0) {

    // Error forking

    perror("lsh");

  } else {

    // Parent process

    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

  }

  return 1;
}

/*
  Execute command
*/

int lsh_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < lsh_num_builtins(); i++) {

    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }

  }

  return lsh_launch(args);
}

/*
  Read line
*/

#define LSH_RL_BUFSIZE 1024

char *lsh_read_line(void)
{
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;

  char *buffer = malloc(sizeof(char) * bufsize);

  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {

    c = getchar();

    if (c == EOF) {

      exit(EXIT_SUCCESS);

    } else if (c == '\n') {

      buffer[position] = '\0';
      return buffer;

    } else {

      buffer[position] = c;

    }

    position++;

    if (position >= bufsize) {

      bufsize += LSH_RL_BUFSIZE;

      buffer = realloc(buffer, bufsize);

      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

/*
  Split line into tokens
*/

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE;
  int position = 0;

  char **tokens = malloc(bufsize * sizeof(char*));

  char *token;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);

  while (token != NULL) {

    tokens[position] = token;
    position++;

    if (position >= bufsize) {

      bufsize += LSH_TOK_BUFSIZE;

      tokens = realloc(tokens, bufsize * sizeof(char*));

      if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }

  tokens[position] = NULL;

  return tokens;
}

/*
  Main shell loop
*/

void lsh_loop(void)
{
  char *line;
  char **args;

  int status;

  do {

    printf("> ");

    line = lsh_read_line();

    add_to_history(line);

    args = lsh_split_line(line);

    status = lsh_execute(args);

    free(line);
    free(args);

  } while (status);
}

/*
  Main function
*/

int main(int argc, char **argv)
{
  lsh_loop();

  return EXIT_SUCCESS;
}