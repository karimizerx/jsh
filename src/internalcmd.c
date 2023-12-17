#include "internalcmd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

char *internals[] = {"pwd", "cd", "exit", "?"};

int exec_pwd()
{
    char curdir[1024];
    if (getcwd(curdir, sizeof(curdir)) != NULL)
    {
        printf("%s\n", curdir);
        return 0;
    }
    else
    {
        perror("jsh: pwd");
        return 1;
    }
}

int exec_cd(char *path)
{
    char curdir[1024]; // pwd of current directory
    getcwd(curdir, sizeof(curdir));
    char newpath[1024]; // The path's directory where we want to go.

    if (path == NULL)
        strcpy(newpath, getenv("HOME"));
    else if (*path == '~')
    {
        // Case : Invalid use of "~" path
        if (*(path + 1) != '/' && *(path + 1) != '\0')
        {
            fprintf(stderr, "\tcd: invalid path's syntax ('%s')\n", path);
            return 1;
        }
        sprintf(newpath, "%s%s", getenv("HOME"), path + 1);
    }
    else if (strcmp(path, "-") == 0)
    {
        // Case : No previous directory
        if (getenv("OLDPWD") == NULL)
        {
            fprintf(stderr, "jsh: cd: \"OLDPWD\" is not define (no previous directory)\n");
            return 1;
        }
        strcpy(newpath, getenv("OLDPWD"));
    }
    else
        strcpy(newpath, path);

    if (chdir(newpath) != 0)
    {
        perror("jsh: cd");
        return 1;
    }
    // Change previous directory
    setenv("OLDPWD", curdir, 1);
    return 0;
}

int exec_exit(int code, command_t *command)
{
    if (command->argc == 2)
    {
        char *end;
        code = strtol(command->argv[1], &end, 10);
        if (*end != '\0')
        {
            fprintf(stderr, "jsh: exit: bad argument\n");
            return 1;
        }
    }
    exit(code);
    return 0;
}

int exec_show_last_return_code()
{
    if (printf("%d\n", jsh.last_exit_code) < 0)
    {
        perror("jsh: ?");
        return 1;
    }
    return 0;
}

int get_signal(char *sig)
{
    for (int i = 1; i < NSIG; i++)
    {
        if (i == 32 || i == 33)
            ;
        else if (!strcmp(strsignal(i), sig))
            return i;
    }
    return -1;
}

int exec_kill(command_t *command)
{
    if (command->argc != 1 && command->argc != 2)
    {
        fprintf(stderr, "jsh: kill: bad argument\n");
        return 1;
    }

    // Get PID
    int isjob = (command->argv[command->argc][0] == '%') ? 1 : 0;
    char *ps = (isjob) ? command->argv[command->argc] + 1 : command->argv[command->argc]; // "PID" or "JOB"
    char *end;
    int pid = strtol(ps, &end, 10);
    if (*end != '\0')
    {
        fprintf(stderr, "jsh: kill: bad argument (%s)\n", ps);
        return 1;
    }
    pid = (isjob) ? -pid : pid;

    // Get SIGNAL
    char *sig = (command->argc == 1) ? "SIGTERM" : command->argv[1] + 1;
    char *end2;

    int signal = strtol(sig, &end2, 10);
    if (*end2 == sig)
    {
        if ((signal = get_signal(sig)) == -1)
        {
            fprintf(stderr, "jsh: kill: bad SIGNAL argument (%s)\n", sig);
            return 1;
        }
    }
    else if (*end2 != '\0') // A lu autre chose qu'un nombre ou qu'un mot
    {
        fprintf(stderr, "jsh: kill: bad SIGNAL argument (%s)\n", sig);
        return 1;
    }

    return (kill(signal, pid) == -1) ? 1 : 0;
}

bool is_internal(char *name)
{
    for (unsigned long i = 0; i < sizeof(internals) / sizeof(char *); ++i)
        if (strcmp(name, internals[i]) == 0)
            return true;
    return false;
}

int exec_internal(command_t *command)
{
    char *cmd = command->argv[0];
    if (strcmp(cmd, "?") == 0)
        return exec_show_last_return_code();
    else if (strcmp(cmd, "pwd") == 0)
        return exec_pwd();
    else if (strcmp(cmd, "cd") == 0)
        return exec_cd(command->argv[1]);
    else if (strcmp(cmd, "kill") == 0)
        return exec_kill(command);
    else if (strcmp(cmd, "exit") == 0)
        return exec_exit(jsh.last_exit_code, command);
    return -1;
}
