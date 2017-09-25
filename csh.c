/* 
Welcome to the Custom Shell (CSH) by Nikhil Pinnaparaju and Sailendra DS.
*/

// Imported Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <setjmp.h>

#define TRUE 1

// Globally Declared Arrays for convenient use
char retrUser[1024];
char host[1024];
char logged_in[1024];
char pwd[1024] = "~";
char input[10240];
char pdir[1000];
char foreproc[100] = {'\0'};

int forepid;
int flag = 0, job_no = 1;
int INFILE = 0;
int OUTFILE = 1;
int SHELLPID;
int ctrlz = 0;
int fg;
int pip_pres = 0;

struct bgproc
{
    pid_t pid;
    char task[100];
};

typedef struct bgproc bgproc;

bgproc procs[1024];

// Function that creates the prompt
void startPrompt()
{
    char *found, *start;

    start = retrUser;

    getcwd(retrUser, 1024);  // Get Current Working Directory
    gethostname(host, 1024); // Get host pc Name

    // printf("%s\n",start);
    int count = 0;

    // Using getlogin was not retrieving the name on my PC
    // Therefore, wrote a while loop to retrieve the username for any PC

    while ((found = strsep(&start, "/")) != NULL)
    {
        // printf("%d:%s\n",count,found);

        if (count == 2)
        {
            strcpy(logged_in, found);
        }

        count++;
    }
}

void printer(int sig);

// Function to break down a command input into an array of pointers based on given delimiter
void parseInput(char *com, char **tokens, char *delim)
{
    char *temp = strtok(com, delim);

    int count = 0;

    while (temp != NULL)
    {
        tokens[count] = temp;
        count++;

        // We pass NULL to 'strtok' because we want to continue iteration of the prev string
        temp = strtok(NULL, delim);
    }
}

// PWD is the inbuilt command to get the present working directory.
// Very easy to implement using getcwd
void runPWD()
{
    char curDir[1024];

    getcwd(curDir, 1024);

    printf("%s", curDir);
}

// Echo is a command that takes in an input and prints it out the same
// However it has some exceptions
// Given single quotes or double quotes they do not get printed
// Hence we check all the characters and don't print the quotes
void runEcho(char **tokens)
{
    int i = 1;
    while (tokens[i] != NULL)
    {
        int j, x = strlen(tokens[i]);
        for (j = 0; j < x; j++)
        {
            if (tokens[i][j] == '\'')
            {
                continue;
            }

            else if (tokens[i][j] == '\"')
            {
                continue;
            }

            else
            {
                printf("%c", tokens[i][j]);
            }
        }

        i++;
    }
    printf("\n");
}

//Helps us in changing directories
void runCD(char **tokens)
{
    int i, j;
    char home_dir[1000];
    strcpy(home_dir, pdir);
    int len = strlen(home_dir);
    if (tokens[1] == NULL) //if no argument is given
    {
        chdir(home_dir); //Go to "home directory"
    }
    else if (tokens[1][0] == '~' || (tokens[1][0] == '.' && tokens[1][1] != '.')) //if full path is given
    {
        for (i = 1; tokens[1][i] != '\0'; i++)
        {
            home_dir[len++] = tokens[1][i]; //copying rest of the path
        }
        home_dir[len++] = '\0';
        chdir(home_dir);
    }
    else if (chdir(tokens[1]) != 0) //for other paths (like ../../)
    {
        perror("Error");
    }
}

// Function to run the ls command
void runLS(char **tokens)
{
    // dir is a struct that is defined in dirent.h
    DIR *cur; // Current Directory;
    struct dirent *file;

    cur = opendir("."); // opens current directory

    // If no flags have been given
    if (tokens[1] == NULL)
    {
        while ((file = readdir(cur)) != NULL)
        {
            // Don't want to print hidden files
            if ((file->d_name)[0] != '.')
                printf("%s ", file->d_name);
        }
        printf("\n");

        closedir(cur);
    }

    // If flags are present
    else
    {
        int i, flags[3], arg = 1;

        for (i = 0; i < 3; i++)
        {
            // printf("%d ",flags[i]);
            flags[i] = 0; // Flags Reset for each Iteration
        }

        while (tokens[arg] != NULL)
        {
            int j, l = strlen(tokens[arg]);

            // printf("%s\n",tokens[arg]);

            // setting of flags into an array flags[]
            for (j = 1; j < l; j++)
            {
                if (tokens[arg][j] == 'a') // If the "a" flag has been selected
                {
                    flags[0] = 1;
                    // printf("%s %d %d\n",tokens[arg],arg,j);
                }

                if (tokens[arg][j] == 'l') // If the "l" flag has been selected
                {
                    flags[1] = 1;
                    // printf("%s %d %d\n",tokens[arg],arg,j);
                }
            }

            arg++;
        }

        // both l and a selected
        if (flags[0] && flags[1])
        {
            struct stat fileStat;

            struct group *grp;
            struct passwd *pass;

            // in the struct tm, numbres represent months and using our array we convert them into corresponding months
            const char *monthnames[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

            int total = 0; // total block size is computed in this var. similar to actual ls -l

            while ((file = readdir(cur)) != NULL)
            {
                stat(file->d_name, &fileStat); // moving data into a struct stat type var

                // file permissions
                printf((S_ISDIR(fileStat.st_mode)) ? "d" : "-");
                printf((fileStat.st_mode & S_IRUSR) ? "r" : "-");
                printf((fileStat.st_mode & S_IWUSR) ? "w" : "-");
                printf((fileStat.st_mode & S_IXUSR) ? "x" : "-");
                printf((fileStat.st_mode & S_IRGRP) ? "r" : "-");
                printf((fileStat.st_mode & S_IWGRP) ? "w" : "-");
                printf((fileStat.st_mode & S_IXGRP) ? "x" : "-");
                printf((fileStat.st_mode & S_IROTH) ? "r" : "-");
                printf((fileStat.st_mode & S_IWOTH) ? "w" : "-");
                printf((fileStat.st_mode & S_IXOTH) ? "x" : "-");
                printf(" ");

                /* Link count.  */
                printf("%lu ", fileStat.st_nlink);

                pass = getpwuid(fileStat.st_uid); // User ID of the file's owner and converting to name using pwd.h
                grp = getgrgid(fileStat.st_gid);  // Group ID of the file's group and converting to name using grp.h

                // printing
                printf("%s ", pass->pw_name);
                printf("%s ", grp->gr_name);
                printf("%ld ", fileStat.st_size);

                const time_t lastModSec = fileStat.st_mtime; // last modified date and time
                struct tm *lastMod = localtime(&lastModSec); // convertion into local time

                printf("%s %d %d:%d ", monthnames[lastMod->tm_mon], lastMod->tm_mday, lastMod->tm_hour, lastMod->tm_min);

                printf("%s ", file->d_name);

                total += fileStat.st_blocks; // total 512 block computation

                printf("\n");
            }
            printf("total %d\n", total / 2); // total 1024 blocks are half our computed value
        }

        // if only a is selected, printing all files including hidden
        else if (flags[0])
        {
            while ((file = readdir(cur)) != NULL)
            {
                printf("%s ", file->d_name);
            }
            printf("\n");
        }

        // if only l is selected, avoiding hidden files and printing as similar to al
        else if (flags[1])
        {
            struct stat fileStat;

            struct group *grp;
            struct passwd *pass;

            const char *monthnames[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

            int total = 0;
            while ((file = readdir(cur)) != NULL)
            {
                if ((file->d_name)[0] != '.')
                {
                    stat(file->d_name, &fileStat);

                    printf((S_ISDIR(fileStat.st_mode)) ? "d" : "-");
                    printf((fileStat.st_mode & S_IRUSR) ? "r" : "-");
                    printf((fileStat.st_mode & S_IWUSR) ? "w" : "-");
                    printf((fileStat.st_mode & S_IXUSR) ? "x" : "-");
                    printf((fileStat.st_mode & S_IRGRP) ? "r" : "-");
                    printf((fileStat.st_mode & S_IWGRP) ? "w" : "-");
                    printf((fileStat.st_mode & S_IXGRP) ? "x" : "-");
                    printf((fileStat.st_mode & S_IROTH) ? "r" : "-");
                    printf((fileStat.st_mode & S_IWOTH) ? "w" : "-");
                    printf((fileStat.st_mode & S_IXOTH) ? "x" : "-");
                    printf(" ");

                    printf("%lu ", fileStat.st_nlink);

                    pass = getpwuid(fileStat.st_uid);
                    grp = getgrgid(fileStat.st_gid);

                    printf("%s ", pass->pw_name);
                    printf("%s ", grp->gr_name);
                    printf("%ld ", fileStat.st_size);

                    const time_t lastModSec = fileStat.st_mtime;
                    struct tm *lastMod = localtime(&lastModSec);

                    printf("%s %d %d:%d ", monthnames[lastMod->tm_mon], lastMod->tm_mday, lastMod->tm_hour, lastMod->tm_min);

                    printf("%s ", file->d_name);

                    total += fileStat.st_blocks;

                    printf("\n");
                }
            }
            printf("total %d\n", total / 2);
        }
    }

    return;
}

// Function to check is argument is & to run in background
int backgroundCheck(char **tokens)
{
    int i = 0;
    for (i = 0; tokens[i] != NULL; i++)
    {
        if (strcmp("&", tokens[i]) == 0) //iterating till we come across "&"
        {

            tokens[i] = NULL;
            return 1; //return 1 if true
        }
    }
    return 0;
}

//Running foreground and background processes
void process(char **com)
{
    int bg = backgroundCheck(com); //Checking if process is a foreground or background process
    pid_t pid;
    int status; //flag to find out wating time for parent process
    pid = fork();
    if (pid < 0)
    {
        perror("Error in forking\n");
    }
    else if (pid == 0) //For child process
    {
        if (execvp(com[0], com) < 0) //Excecuting child process
        {
            perror("Error ");
            exit(0);
        }
    }
    else
    {
        if (bg == 0) //foreground process
        {
            wait(&status); //Waiting for child process to finish
        }
        else if (bg == 1) //BAckground process
        {
            printf("Process started: %s [%d]\n", com[0], pid);
        }
    }
}

void proc_exit()
{
    union wait wstat;
    pid_t pid;

    while (TRUE)
    {
        pid = wait3(&wstat, WNOHANG, (struct rusage *)NULL);
        if (pid == 0)
            return;
        else if (pid == -1)
            return;
        else
        {
            char *cat[100] = {NULL};

            cat[0] = "cat";
            cat[2] = "/proc/";
            cat[3] = "/cmdline";

            char proc[10];

            // string appending to get the string proc/[pid]/status
            sprintf(proc, "%d", pid);

            // printf("%s\n",proc);

            int i, l1 = strlen(cat[2]), l2 = strlen(proc);

            char *concat = (char *)malloc(9 + l1 + l2);

            strcat(concat, cat[2]);
            strcat(concat, proc);
            strcat(concat, cat[3]);

            // printf("%s\n",concat);

            cat[1] = malloc(sizeof(concat) + 1);

            strcpy(cat[1], concat);

            // // printf("%s\n",cat[1]);

            cat[2] = '\0';
            cat[3] = '\0';

            // process(cat);

            printf("Process with PID: %d exited with Return code: %d\n", pid, wstat.w_retcode);
            fflush(stdout);
            // prompt();
        }
    }
}

// User defined command pinfo
void runPinfo(char **tokens)
{
    int stand;
    char *procStat;
    // int s = strlen(procStat);

    // if no argument is given, pinfo of self is run
    if (tokens[1] == NULL)
    {
        // the info we want is stored in proc/self/status
        procStat = "cat proc/self/status";

        char *cat[100] = {NULL};

        cat[0] = "cat";
        cat[1] = "/proc/self/status";

        // execute cat through process function
        process(cat);
    }

    // if argument is given
    else
    {
        // the info we want is in proc/[pid]/status
        char *cat[100] = {NULL};

        cat[0] = "cat";
        cat[2] = "/proc/";
        cat[3] = "/status";

        // string appending to get the string proc/[pid]/status
        char *proc = tokens[1];

        int i, l1 = strlen(cat[2]), l2 = strlen(tokens[1]);

        char *concat = (char *)malloc(8 + l1 + l2);

        strcat(concat, cat[2]);
        strcat(concat, tokens[1]);
        strcat(concat, cat[3]);

        // printf("%s\n",concat);

        cat[1] = malloc(sizeof(concat) + 1);

        strcpy(cat[1], concat);

        // printf("%s\n",cat[1]);

        cat[2] = '\0';
        cat[3] = '\0';

        // execute cat through process function
        process(cat);
    }
}

void runNightswatch(char **tokens)
{
    // printf("OKay");
    char *watch[100] = {NULL};

    watch[0] = "watch";
    watch[1] = tokens[1];
    watch[2] = tokens[2];

    // char ex;
    // int tim;

    // while (scanf("%c",&ex) != 'q')
    // {

    watch[3] = "cat";
    watch[4] = "/proc/meminfo";
    watch[5] = "|";
    watch[6] = "grep";
    watch[7] = "Dirty";

    //     tim = atoi(&ex);

    process(watch);

    //     sleep(tim);
    // }
}

void runSetEnv(char **tokens)
{
    int suc = setenv(tokens[1], tokens[2], 1);

    // printf("done %d\n", getenv(tokens[1]));
}

void runUnsetEnv(char **tokens)
{
    int suc = unsetenv(tokens[1]);

    // printf("done %d\n", suc);
}

void redirection()
{
    if (INFILE != 0)
    {
        dup2(INFILE, STDIN_FILENO);
        close(INFILE);
    }
    if (OUTFILE != 1)
    {
        dup2(OUTFILE, STDOUT_FILENO);
        close(OUTFILE);
    }

    return;
}

void runJobs()
{
    int i;
    if (job_no == 1)
    {
        printf("No Background Processes\n");
        return;
    }

    for (i = 1; i < job_no; i++)
    {
        char *state;
        int st = kill(procs[i].pid, 0); // returns 0 if proc exists

        if (!st)
            state = "Running";

        else
            state = "Stopped";

        printf("[%d] %s %s[%d]\n", i, state, procs[i].task, procs[i].pid);
    }

    return;
}

void removeJob(int job_id)
{
    printf("%d", job_id);

    int i = 1;

    while (procs[i].pid != job_id)
    {
        i++;
    }

    while (i < job_no)
    {
        if (procs[i + 1].task != NULL)
        {
            strcpy(procs[i].task, procs[i + 1].task);
            procs[i].pid = procs[i + 1].pid;
        }

        i++;
    }

    strcpy(procs[i].task, NULL);
    procs[i].pid = 0;

    return;
}

void runKjob(char **tokens)
{
    if (tokens[2] == NULL)
        perror("Too few arguments given\n");

    else
    {
        int curjob = atoi(tokens[1]);
        int sigsend = atoi(tokens[2]);

        if (job_no < curjob)
            perror("Job not available");

        else
        {
            kill(procs[curjob].pid, sigsend);

            if (sigsend == 9)
            {
                removeJob(procs[curjob].pid);
                job_no--;
            }
        }
    }
}

void runOverkill()
{
    int i;

    if (job_no < 2)
    {
        perror("No Background jobs to kill.\n");
        return;
    }

    for (i = 1; i < job_no; i++)
    {
        kill(procs[i].pid, 9);
        // signal(SIGCHLD,printer);
    }

    job_no = 1;

    return;
}

void runFG(char **args)
{
    int status;
    if (args[1] != NULL)
    {
        int idx = atoi(args[1]);
        if (idx >= job_no || idx < 0)
            printf("Specified job does not exist\n");
        else
        {
            if (kill(procs[idx].pid, SIGCONT) < 0)
            {
                perror("kill(SIGCONT)"); // sending a signal to run in foreground
                return;
            }

            int chd_pid = procs[idx].pid;

            forepid = chd_pid;
            strcpy(foreproc, procs[idx].task);

            printf("Process in foreground\n");
            signal(SIGINT, proc_exit);
            signal(SIGTSTP, printer);
            waitpid(chd_pid, &status, WCONTINUED);
            printf("Exited from fg\n");
        }
    }
    else
    {
        printf("No job number specified\n");
    }
    return;
}

void runBG(char **args)
{
    int status;
    if (args[1] != NULL)
    {
        int idx = atoi(args[1]);
        if (idx >= job_no || idx < 0)
            printf("Specified job does not exist\n");
        else
        {
            if (kill(procs[idx].pid, SIGCONT) < 0)
            {
                perror("kill(SIGCONT)"); // sending a signal to run in background
                return;
            }

            int chd_pid = procs[idx].pid;

            printf("Process in background\n");
            signal(SIGINT, proc_exit);
        }
    }

    else
    {
        printf("No job number specified\n");
    }
    return;
}

void executeCom() //Main command executing function
{
    char *after[100] = {NULL};
    char *multiPipe[100] = {NULL};
    parseInput(input, multiPipe, ";");
    int i = 0, k = 0, bg = 0;
    pip_pres = 0;
    while (multiPipe[i] != NULL)
    {
        // parseInput(multiPipe[i], after, "|");
        after[i] = multiPipe[i];
        int j = 0;
        int fd[2];

        while (after[j] != NULL)
        {
            int redir_size = strlen(after[j]);
            char *out = NULL, *in = NULL;
            char *com[100] = {NULL};

            parseInput(after[j], com, " ");

            k = 0;
            int red;

            int edit = 0; // 0 for out rewrite and 1 for out append

            while (com[k] != NULL)
            {
                red = 0;
                if (com[k] != NULL && !strcmp(com[k], "<"))
                {
                    if (com[k + 1] != NULL)
                    {
                        in = com[k + 1];
                    }

                    else
                        perror("No specified input file");

                    int temp = k;

                    while (com[temp] != NULL)
                    {
                        com[temp] = com[temp + 2];
                        temp++;
                    }
                    com[temp - 1] = NULL;
                    com[temp - 2] = NULL;
                    red = 1;
                }

                if (com[k] != NULL && (!strcmp(com[k], ">") || !strcmp(com[k], ">>")))
                {
                    if (!strcmp(com[k], ">>"))
                        edit = 1;

                    if (com[k + 1] != NULL)
                    {
                        out = com[k + 1];

                        int temp = k;
                        while (com[temp] != NULL)
                        {
                            com[temp] = com[temp + 2];
                            temp++;
                        }
                        com[temp - 1] = NULL;
                        com[temp - 2] = NULL;
                    }
                    else
                    {
                        OUTFILE = 1;
                        int temp = k;
                        while (com[temp] != NULL)
                        {
                            com[temp] = com[temp + 1];
                            temp++;
                        }
                        com[temp - 1] = NULL;
                    }
                    red = 1;
                }

                if (!strcmp(com[k],"|"))
                {
                    pip_pres = 1;
                }

                k++;
            }

            if (pip_pres)
            {
                char *depipe[100] = {NULL};

                int te = 0,count = 0,counter=0;
                while (com[te]!=NULL)
                {
                    if (strcmp(com[te],"|"))
                    {
                        depipe[count] = malloc(256);
                        depipe[count] = strcat(depipe[count]," ");
                        depipe[count] = strcat(depipe[count],com[te]);
                    }

                    else
                    {
                        count++;
                    }

                    te++;
                }

                int a=0;

                while (depipe[a]!=NULL)
                {
                    printf("%s\n",depipe[a]);
                    a++;
                }

                int p[2];
                pid_t pid;
                int fd_in = 0,m=0;
            
                while (depipe[m] != NULL)
                {
                    pipe(p);
                    if ((pid = fork()) == -1)
                    {
                        exit(EXIT_FAILURE);
                    }
                    else if (pid == 0)
                    {
                        dup2(fd_in, 0); //change the input according to the old one
                        if (depipe[k + 1] != NULL)
                            dup2(p[1], 1);
                        close(p[0]);
                        execvp(depipe[0], depipe);
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        wait(NULL);
                        close(p[1]);
                        fd_in = p[0]; //save the input for the next command
                        m++;
                    }
                }

                return;
            }

            if (in != NULL)
                INFILE = open(in, O_RDONLY);

            if (out != NULL)
                if (!edit)
                    OUTFILE = open(out, O_TRUNC | O_WRONLY | O_CREAT, S_IRWXU);
                else
                    OUTFILE = open(out, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);

            int save_out = dup(fileno(stdout));
            int save_in = dup(fileno(stdin));

            dup2(OUTFILE, fileno(stdout));
            dup2(INFILE, fileno(stdin));

            if (com[0] == NULL)
                return;
            //printf("red = %d\n",red);

            if (OUTFILE == 1)
                signal(SIGCHLD, proc_exit);

            int bg = backgroundCheck(com);

            if (!strcmp("pwd", com[0]))
            {
                runPWD();
                printf("\n");
            }

            else if (!strcmp("echo", com[0]))
            {
                runEcho(com);
                break;
            }

            if (!strcmp("cd", com[0]))
            {
                runCD(com);
            }

            else if (!strcmp("ls", com[0]))
            {
                runLS(com);
            }

            else if (!strcmp("clear", com[0]))
            {
                system("clear");
            }

            else if (!strcmp("quit", com[0]))
            {
                _exit(0); // _exit is encapsulation of the exit command as it should not be used improperly
            }

            else if (!strcmp("pinfo", com[0]))
            {
                runPinfo(com);
            }

            else if (!strcmp("nightswatch", com[0]))
            {
                runNightswatch(com);
            }

            else if (!strcmp("setenv", com[0]))
            {
                runSetEnv(com);
            }

            else if (!strcmp("unsetenv", com[0]))
            {
                runUnsetEnv(com);
            }

            else if (!strcmp("jobs", com[0]))
            {
                runJobs();
            }

            else if (!strcmp("kjob", com[0]))
            {
                runKjob(com);
            }

            else if (!strcmp("overkill", com[0]))
            {
                runOverkill();
            }

            else if (!strcmp("fg", com[0]))
            {
                runFG(com);
            }

            else if (!strcmp("bg", com[0]))
            {
                runBG(com);
            }

            else
            {
                pipe(fd);

                pid_t pid;

                int status; //flag to find out wating time for parent process

                pid = fork();
                if (pid < 0)
                {
                    perror("Error in forking\n");
                }
                else if (pid == 0) //For child process
                {
                    pid_t chd = getpid();

                    forepid = chd;

                    if (after[j + 1] != NULL)
                    {
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[1]);
                    }

                    strcpy(foreproc, com[0]);

                    if (execvp(com[0], com) < 0) //Excecuting child process
                    {
                        perror("Error ");
                        exit(0);
                    }
                }
                else
                {
                    fg = 1 - bg;
                    if (bg == 0) //foreground process
                    {
                        // wait(&status); //Waiting for child process to finish
                        waitpid(-1, &status, WUNTRACED);
                    }
                    else if (bg == 1) //Background process
                    {
                        procs[job_no].pid = pid;
                        strcpy(procs[job_no].task, com[0]);

                        // printf("task: %s\n",procs[job_no].task);

                        job_no++;

                        printf("Process started: %s [%d]\n", com[0], pid);
                    }
                    INFILE = fd[0];
                    close(fd[1]);
                }
            }

            j++;

            fflush(stdout);
            close(OUTFILE);

            fflush(stdin);
            close(INFILE);

            dup2(save_out, fileno(stdout));
            close(save_out);

            dup2(save_in, fileno(stdin));
            close(save_in);

            save_in = 0, save_out = 0;
            in = NULL, out = NULL;
            INFILE = 0;
            OUTFILE = 1;
        }
        i++;
    }

    for (i = 0; i < 100; i++)
    {
        if (multiPipe[i] != NULL)
        {
            *multiPipe[i] = '\0';
        }
    }
}

void prompt()
{
    fflush(stdout);

    int i, j;

    printf("%s@%s:", logged_in, host); //prompt printing
    char curDir[1024];
    getcwd(curDir, 1024);
    for (i = 0; curDir[i] != '\0'; i++)
    {
        if (pdir[i] == curDir[i])
            continue;
        else
            break;
    }
    if (i == strlen(pdir))
    {
        printf("~");
        for (j = i; curDir[j] != '\0'; j++)
            printf("%c", curDir[j]);
    }
    else
        runPWD();
    printf(">");
}

void printer(int sig)
{
    // printf("signal number %d\n", sig);

    if (sig == 2) // SIGINT - CTRL + c
    {
        printf("\n");

        prompt();
        fflush(stdout); // flush of stdout stream prevents the buffering by OS of stdout
    }                   // output is written to terminal only after newline is encountered this prevents that

    if (sig == 3) // SIGQUIT - CTRL + D
    {
        printf("\n");

        // prompt();
        // fflush(stdout);
    }

    if (sig == 20) // SIGTSTP - CTRL + Z
    {

        printf("%s %d\n", foreproc, forepid);

        if (foreproc != NULL)
        {
            strcpy(procs[job_no].task, foreproc);
        }

        if (forepid != 0)
        {
            procs[job_no].pid = forepid;
            kill(forepid, SIGSTOP);
            job_no++;
        }

        printf("\n");

        prompt();
        fflush(stdout);
    }

    if (sig == 17) // SIGCHLD - Child proc stopped
    {
        if (sig == SIGCHLD)
        {
            union wait wstat;
            pid_t pid;

            while (1)
            {
                pid = wait3(&wstat, WNOHANG, (struct rusage *)NULL);
                if (pid == 0)
                    return;
                else if (pid == -1)
                    return;
                else
                {
                    // fprintf(stderr,"\nProcess with PID : %d exited with return value: %d\n",pid,wstat.w_retcode);
                    removeJob(pid);
                }
            }
        }
    }

    return;
}

int main()
{
    SHELLPID = getpid();

    getcwd(pdir, 1000);
    startPrompt();
    while (1)
    {
        signal(SIGINT, printer);
        signal(SIGQUIT, printer);
        signal(SIGTSTP, printer);

        forepid = 0;
        pip_pres = 0;

        prompt();

        if (scanf("%[^\n]s", input) != EOF)
        {
            getchar();
        }

        else
        {
            printf("\n");
        }

        executeCom();
    }

    return 0;
}