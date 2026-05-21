/***************************************************************************//**

  @file         main.c

  @author      Mahmoud Ragab
                Based on Stephen Brennan's lsh

  @date           may 2026

  @brief                Minimal Unix Shell implemented in C


*******************************************************************************/

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
==========================
ADDED
==========================
*/
extern char **environ;

#define HISTORY_SIZE 100

char *history[HISTORY_SIZE];
int history_count = 0;

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_pwd(char **args);
int lsh_echo(char **args);
int lsh_history(char **args);
int lsh_env(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
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
  Builtin function implementations.
*/

/**
   @brief Builtin command: change directory.
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

/**
   @brief Builtin command: print help.
 */
int lsh_help(char **args)
{
  int i;
  printf("Mahmoud ragab's LSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < lsh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

/**
   @brief Builtin command: exit.
 */
int lsh_exit(char **args)
{
  return 0;
}

int lsh_pwd(char **args)
{
  char cwd[1024];

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("lsh");
  }

  return 1;
}


int lsh_echo(char **args)
{
  int i = 1;

  while (args[i] != NULL) {
    printf("%s ", args[i]);
    i++;
  }

  printf("\n");

  return 1;
}

int lsh_history(char **args)
{
  int i;

  for (i = 0; i < history_count; i++) {
    printf("%d %s\n", i + 1, history[i]);
  }

  return 1;
}

int lsh_env(char **args)
{
  int i = 0;

  while (environ[i] != NULL) {
    printf("%s\n", environ[i]);
    i++;
  }

  return 1;
}

/**
  @brief Launch a program and wait for it to terminate.
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

/**
   @brief Execute shell built-in or launch program.
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

/**
   @brief Read a line of input from stdin.
 */
char *lsh_read_line(void)
{
#define LSH_RL_BUFSIZE 1024

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

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

/**
   @brief Split a line into tokens.
 */
char **lsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE;
  int position = 0;

  char **tokens = malloc(bufsize * sizeof(char*));

  char *token;
  char **tokens_backup;

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

      tokens_backup = tokens;

      tokens = realloc(tokens, bufsize * sizeof(char*));

      if (!tokens) {

        free(tokens_backup);

        fprintf(stderr, "lsh: allocation error\n");

        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }

  tokens[position] = NULL;

  return tokens;
}

/**
   @brief Loop getting input and executing it.
 */
void lsh_loop(void)
{
  char *line;
  char **args;

  int status;

  do {

    printf("> ");

    line = lsh_read_line();


    if (history_count < HISTORY_SIZE) {
      history[history_count] = strdup(line);
      history_count++;
    }

    args = lsh_split_line(line);

    status = lsh_execute(args);

    free(line);
    free(args);

  } while (status);
}

/**
   @brief Main entry point.
 */
int main(int argc, char **argv)
{
  lsh_loop();

  return EXIT_SUCCESS;
}