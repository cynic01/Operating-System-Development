#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))

/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_pwd, "pwd", "prints the current working directory"},
    {cmd_cd, "cd", "changes the current working directory"},
};

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

/* Prints the current working directory */
int cmd_pwd(unused struct tokens* tokens) {
  char *pwd = getcwd(NULL, 0);
  printf("%s\n", pwd);
  free(pwd);
  return 0;
}

/* Changes the current working directory to the specified path */
int cmd_cd(struct tokens* tokens) {
  size_t len_tokens = tokens_get_length(tokens);
  if (len_tokens > 2) {
    printf("too many arguments");
    return -1;
  } else if (len_tokens == 1) {
    return chdir(getenv("HOME"));
  } else {
    int ret = chdir(tokens_get_token(tokens, 1));
    if (ret == 0) return 0;
    perror("chdir");
    return -1;
  }
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);

    signal(SIGTTOU, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
  }
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);

  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) { // run built-in commands
      cmd_table[fundex].fun(tokens);
    } else { // run commands as programs
      pid_t pid = fork();
      if (pid == 0) { // child process
        setpgid(0, 0);
        signal(SIGTTOU, SIG_IGN);
        if (tcsetpgrp(0, getpid())) perror("tcsetpgrp");
        signal(SIGTTOU, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        // process arguments (__argv)
        size_t len_tokens = tokens_get_length(tokens);
        // bool redir_input = false, redir_output = false;
        size_t arg_count = 0;
        char *args[len_tokens + 1];
        for (int i = 0; i < len_tokens; i++) {
          char *token = tokens_get_token(tokens, i);
          if (strcmp(token, "<") == 0) { // redirect input
            // redir_input = true;
            int fd = open(tokens_get_token(tokens, i+1), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            dup2(fd, 0);
            close(fd);
            i++; // skip as we have processed it
          } else if (strcmp(token, ">") == 0) { // redirect output
            // redir_output = true;
            int fd = open(tokens_get_token(tokens, i+1), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            dup2(fd, 1);
            close(fd);
            i++; // skip as we have processed it
          } else if (strcmp(token, "|") == 0) { // redirect output of parent, input of child
            int pipefd[2];
            pipe(pipefd);
            pid_t pid_pipe = fork();
            if (pid_pipe == 0) { // child process
              setpgid(0, 0);
              signal(SIGTTOU, SIG_IGN);
              if (tcsetpgrp(0, getpid())) perror("tcsetpgrp");
              signal(SIGTTOU, SIG_DFL);
              signal(SIGINT, SIG_DFL);
              signal(SIGTSTP, SIG_DFL);
              close(pipefd[1]);
              dup2(pipefd[0], 0);
              close(pipefd[0]);
              for (size_t j = 0; j < arg_count; j++) args[j] = NULL;
              arg_count = 0;
            } else { // parent process
              close(pipefd[0]);
              dup2(pipefd[1], 1);
              close(pipefd[1]);
              break;
            }
          } else { // normal arguments
            args[arg_count] = token;
            arg_count++;
          }
        }
        args[arg_count] = NULL;

        // process program path resolution (__path)
        char *prog = args[0];
        if (prog[0] == '/') { // absolute path
          execv(prog, args);
        } else if (prog[0] == '.') { // path in CWD
          char *real_path = realpath(prog, NULL);
          if (real_path) {
            execv(real_path, args);
            free(real_path);
          } else {
            printf("Error resolving path\n");
            exit(-1);
          }
        } else { // find from PATH env
          char *path_env = getenv("PATH");
          struct tokens* paths = tokenize(path_env);
          for (size_t i = 0; i < tokens_get_length(paths); i++) {
            // build the absolute path string
            char abs_path[4096];
            char *dir = tokens_get_token(paths, i);
            strcpy(abs_path, dir);
            abs_path[strlen(dir)] = '/';
            abs_path[strlen(dir) + 1] = '\0';
            strcat(abs_path, prog);
            // check if the program exists
            if (access(abs_path, F_OK) == 0) {
              tokens_destroy(paths);
              execv(abs_path, args);
            }
          }
        }

        // handle error cases
        perror("exec");
        exit(-1);
      } else { // parent process
        waitpid(pid, NULL, 0);
        if (tcsetpgrp(0, getpgid(0))) perror("tcsetpgrp");
      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
