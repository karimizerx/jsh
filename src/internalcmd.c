#include "internalcmd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int pwd()
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

int cd(char *path)
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
        fprintf(stderr, "%s\n", getenv("OLDPWD"));
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

int quit(int code, command_t *command)
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