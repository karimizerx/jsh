#include "input.h"
#include "vector.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "jobs.h"

// Global Variables - Colors
char *vert = "\001\033[32m\002";
char *cyan = "\001\033[36m\002";
char *jaune = "\001\033[33m\002";
char *bleu = "\001\033[34m\002";
char *rouge = "\001\033[91m\002";
char *normal = "\001\033[00m\002";

char **split_string(char *chemin, char *separateur)
{
    char *chem = strdup(chemin);
    int size = 1;
    for (int i = 0; i < strlen(chem); ++i)
        if (chem[i] == separateur[0])
            ++size;
    char **composantes = calloc(size + 2, sizeof(char *));
    int i = 0;
    for (char *mot = strtok(chem, separateur); mot != NULL; mot = strtok(NULL, separateur))
    {
        composantes[i] = strdup(mot);
        i++;
    }
    free(chem);
    composantes[size + 1] = NULL;
    return composantes;
}

int nbchiffres(int nb)
{
    int i = 1;
    while ((nb = nb / 10) > 0)
        ++i;
    return i;
}

command_t *parse_command(char *read){
    command_t *out = calloc(1, sizeof(command_t));
    char *line = strdup(read);
    char **parts = split_string(read, " ");
    vector argv = vector_empty();

    char *symbols[] = {"<", ">", ">|", ">>", "2>", "2>|", "2>>"};
    command_redir_type_t flags[] = { R_INPUT, R_NO_CLOBBER, R_CLOBBER, R_APPEND, R_NO_CLOBBER, R_CLOBBER, R_APPEND };
    command_redir_t *target[] = { &out->stdin,  &out->stdout,  &out->stdout,  &out->stdout,  &out->stderr, &out->stderr,  &out->stderr };

    for (int i = 0; parts[i] != NULL; ++i)
    {

        if (strcmp(parts[i], "&") == 0 && parts[i+1] == NULL){
            line[strlen(line)-2] = '\0';
            out->bg = true;
            continue;
        }

        bool redir = false;
        for (int j=0; j<sizeof(symbols)/sizeof(char*); ++j){
            if (strcmp(parts[i], symbols[j]) == 0 && parts[i+1] != NULL) {
                target[j]->type = flags[j];
                target[j]->path = strdup(parts[i+1]);
                i += 1;
                redir = true;
                break;
            }
        }

        if (redir) continue;

        vector_append(&argv, strdup(parts[i]));
    }

    for (int i=0; parts[i] != NULL; ++i) free(parts[i]);
    free(parts);

    vector_append(&argv, NULL);
    vector_shrink(&argv);
    out->argc = vector_length(&argv) - 1;
    out->argv = (char**)argv.data;
    out->line = line;

    return out;
}

command_t *read_command()
{
    // Previous calculations
    char pwd[1024];
    getcwd(pwd, sizeof(pwd));
    char curdir[strlen(pwd) + 1];
    getcwd(curdir, sizeof(curdir));
    long int nbjobs = job_count(&jsh.processes);
    unsigned int nbcj = nbchiffres(nbjobs);

    // Formatted prompt
    char prompt[30];
    unsigned int lenprompt = 4 + nbcj; // 4 for "[]$ " & nbcj for nbjobs
    if (strlen(curdir) + lenprompt > 30)
        sprintf(prompt, "%s[%ld]%s...%s%s$ ", cyan, nbjobs, vert, (curdir + (strlen(curdir) - 30 + lenprompt + 3)), normal);
    else
        sprintf(prompt, "%s[%ld]%s%s%s$ ", cyan, nbjobs, vert, curdir, normal);

    char *read = readline(prompt);
    if (read == NULL) { return parse_command("exit");}
    if (strcmp(read, "") == 0) { free(read); return NULL; }
    add_history(read);

    command_t *out = parse_command(read);
    free(read);
    return out;
}
