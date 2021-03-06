#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "LineParser.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

void printCommand(cmdLine* c);

void execute(cmdLine *pCmdLine);
void changeDirectory(char* dir);
#define max_INPUT 2048
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define TERMINATED  -1
#define SUSPENDED 0
#define RUNNING 1

typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                  /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	                  /* next process in chain */
} process;

typedef struct vars{
    char *key;
    char *value;
    struct vars *next;
} vars;

void addProcess(process** process_list, cmdLine* cmd, pid_t pid); /* Receive a process list (process_list), a command (cmd), and the process id (pid) of the process running the command. */
void printProcessList(process** process_list); /*print the processes. */
char* statusString(int status);
// void terPro(cmdLine* cmd);
void upgrade(process** procs,int num);
void handler(int sig);
void freeProcessList(process* process_list);
void updateProcessStatus(process* process_list, int pid, int status);
void updateProcessList(process **process_list);

void printVariables();
char *searchVar(char* key);
void freeVars(vars *va);

process** procs;
vars *variables;
int main() {
    char cwd[PATH_MAX];
    char input[2048];
    cmdLine* command;
    procs = malloc(sizeof(process*));
    procs[0] = NULL;
    variables = NULL;
    signal(SIGCHLD,handler); /*signal to know if a child has finished*/
    /*signal(SIGINT,handler); //the three signals of task2c
    signal(SIGCONT,handler);
    signal(SIGTSTP,handler);*/
    int SKW(cmdLine* cmd);

   while(1)
   {
       if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s$ ", cwd);
            fgets(input,max_INPUT,stdin);
            // printf("??? %s\n",input);
            
            if(!strncmp(input,"procs",5))
            {
                printProcessList(procs);
                continue;
            }
            if(!strncmp(input,"quit",4)) break;
            if(strlen(input) == 1) continue;/* new line */
            command = parseCmdLines(input);
            if(SKW(command)==1) continue;
            // printCommand(command);
            execute(command);
            // freeCmdLines(command);
        } else {
            perror("getcwd() error");
            return 1;
        }
   }
   freeProcessList(procs[0]);
   freeVars(variables);
   return 0;
}
void freeVars(vars *va){
    if(va->next != NULL)
    {
        freeVars(va->next);
    }
    free(va->value);
    free(va->key);
    free(va);
}

void printCommand(cmdLine* c)
{
    printf("argCount: %d\n",c->argCount);
    printf("arguments: \n");
    for(int i=0;i<c->argCount;i++)
    {
        printf("%s\n",c->arguments[i]);
    }
    printf("inputRedirect: %s\n",c->inputRedirect);
    printf("outputRedirect: %s\n",c->outputRedirect);
    printf("index: %d\n",c->idx);
    printf("blocking: %c\n",c->blocking);
    if(c->next!=NULL) printCommand(c->next);
}

void printVariables()
{
    vars* tmp=variables;
    while(tmp!=NULL)
    {
        printf("%-8s %s\n",tmp->key,tmp->value);
        tmp=tmp->next;
    }
}

void execute(cmdLine *pCmdLine)
{
    int debug = 0, cd = 0,set=0,var=0;
    char* dir;
    for(int i=0;i<pCmdLine->argCount;i++){
        if(strncmp(pCmdLine->arguments[i],"-d",2)==0) 
        {
            debug=1;
        }else if(strncmp(pCmdLine->arguments[i],"cd",2)==0) 
        {
            cd=1;
            if(strncmp(pCmdLine->arguments[i+1],"~",1)==0)
            {
                dir = getenv("HOME");
            }else
            {
                dir = pCmdLine->arguments[i+1];
            }
        }else if(strncmp(pCmdLine->arguments[i],"set",3)==0)
        {
            set =1;
        }else if(strncmp(pCmdLine->arguments[i],"vars",4)==0)
        {
            var=1;
        }
    }
    if (var==1)
    {
        printVariables();
        if(pCmdLine->next!=NULL) execute(pCmdLine->next);
        return;
    }
    if(cd==1)
    {
         changeDirectory(dir);
         addProcess(procs,pCmdLine,getpid());
         if(pCmdLine->next!=NULL) execute(pCmdLine->next);
         return;
    }
    if (set==1)
    {
        if (variables == NULL)
        {
            variables=malloc(sizeof(vars));
            variables->next=NULL;
            variables->key=malloc(sizeof(char)*strlen(pCmdLine->arguments[1]));
            strcpy(variables->key,pCmdLine->arguments[1]);
            variables->value=malloc(sizeof(char)*strlen(pCmdLine->arguments[2]));
            strcpy(variables->value,pCmdLine->arguments[2]);
            if(pCmdLine->next!=NULL) execute(pCmdLine->next);
            return;
        }
        vars* curr = variables;
        while(curr->next!=NULL)
        {
            if (!strcmp(curr->key,pCmdLine->arguments[1]))
            {
                free(curr->value);
                curr->value = malloc(sizeof(char)*strlen(pCmdLine->arguments[2]));
                strcpy(curr->value,pCmdLine->arguments[2]);
                if(pCmdLine->next!=NULL) execute(pCmdLine->next);
                return;
            }
            curr=curr->next;
        } 

        if (!strcmp(curr->key,pCmdLine->arguments[1]))/* the last node */
        {
            free(curr->value);
            curr->value=malloc(sizeof(char)*strlen(pCmdLine->arguments[2]));
            strcpy(curr->value,pCmdLine->arguments[2]);
            if(pCmdLine->next!=NULL) execute(pCmdLine->next);
            return;
        }
        curr->next=malloc(sizeof(vars));
        curr=curr->next;
        curr->next=NULL;
        curr->key=malloc(sizeof(char)*strlen(pCmdLine->arguments[1]));
        strcpy(curr->key,pCmdLine->arguments[1]);
        curr->value=malloc(sizeof(char)*strlen(pCmdLine->arguments[2]));
        strcpy(curr->value,pCmdLine->arguments[2]);
        if(pCmdLine->next!=NULL) execute(pCmdLine->next);
        return;
    }

    for(int i=0;i<pCmdLine->argCount;i++) /* replace the variables */
    {
        if(strncmp(pCmdLine->arguments[i],"$",1) == 0)
        {
            char * rep = searchVar(&pCmdLine->arguments[i][1]);
            //change the arguesme
            if (rep==NULL)
            {
                fprintf(stderr,"Activating a variable that does not exist.\n");
                return;
            }
            replaceCmdArg(pCmdLine,i,rep);
        }
    }

    int f=fork();
    int fdInput,fdOut;
    if(f==0)
    {
        if(debug)
        {
            fprintf(stderr,"Child => PPID: %d PID: %d\n", getppid(), getpid());
            fprintf(stderr,"executing Commands:\n");
            for(int i=0;i<pCmdLine->argCount;i++)
            {
                fprintf(stderr,"%s\n",pCmdLine->arguments[i]);
            }
        }
        if (pCmdLine->inputRedirect==NULL && pCmdLine->outputRedirect==NULL)
        {
            f = execvp(pCmdLine->arguments[0],pCmdLine->arguments);
            if(f==-1)
            {
                perror(pCmdLine->arguments[0]);
                _exit(EXIT_FAILURE);
            } 
        }
        else 
        {
            if(pCmdLine->inputRedirect!=NULL)
            {
                if((fdInput = open(pCmdLine->inputRedirect,O_RDONLY))<0)
                {
                    fprintf(stderr,"ERROR: cann't open fileto read\n");
                    exit(0);
                }
                dup2(fdInput,0);
                close(fdInput);
            }
            if (pCmdLine->outputRedirect!=NULL)
            {
                if((fdOut = open(pCmdLine->outputRedirect,O_CREAT|O_WRONLY|O_TRUNC,644))<0)
                {
                    fprintf(stderr,"ERROR: cann't open fileto write\n");
                    exit(0);
                }
                dup2(fdOut,1);
                close(fdOut);
            }
            f = execvp(pCmdLine->arguments[0],pCmdLine->arguments);
            if(f==-1)
            {
                perror(pCmdLine->arguments[0]);
                _exit(EXIT_FAILURE);
            }
        }

    }
    else
    {
        addProcess(procs,pCmdLine,f);
        if(debug){
            fprintf(stderr,"Parent => PID: %d\n", getpid());
            // fprintf(stderr,"Waiting for child process to finish.\n");
            wait(NULL);
            // fprintf(stderr,"Child process finished.\n");
        }else{
            if(pCmdLine->blocking==1){
            int status;
            waitpid(f, &status, 0);
            }
        }
    }
    if(f == -1){
        perror("EXECUTION failed\n");
        _exit(1);
    }
    if(pCmdLine->next!=NULL) execute(pCmdLine->next);
}

void changeDirectory(char* dir)
{
    if(chdir(dir)==-1)
    {
        fprintf(stderr, "cd failed\n");
    }else
    {
        char cwd[PATH_MAX];
        fprintf(stderr, "we got a new directory: %s\n",getcwd(cwd, sizeof(cwd)));
    }
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid) 
{
    process* toAdd = malloc(sizeof(process));
    toAdd->cmd=cmd;
    toAdd->pid=pid;
    toAdd->status = RUNNING;
    toAdd->next=NULL;

    if(process_list[0]==NULL)/* if the process ;list is empty */
    {
        // printf("got here\n");
        process_list[0]=toAdd;
    }
    else/* add to the end of the list */
    {
        // printf("we are here\n");
        process* tmp=process_list[0];
        while(tmp->next!=NULL)
        {
            tmp=tmp->next;
        }
        tmp->next=toAdd;
    }    
}

void printProcessList(process** process_list)
{
    if(!process_list) return;
    printf("PID       Command      STATUS\n");
    updateProcessList(process_list);
    process* curr=process_list[0];
    while(curr!=NULL)
    {
        cmdLine* command=curr->cmd;
        while(command!=NULL)
        {
            printf("%-10d %-13s %s\n",curr->pid,command->arguments[0],statusString(curr->status));
            command=command->next;
        }
        curr = curr->next;
    }
}

char* statusString(int status)
{
    static char* statuses[] = {"Terminated", "Suspended", "Running"};
    return statuses[status + 1];
}

void updateProcessList(process **process_list)
{
    process* list=process_list[0];
    process* prev=process_list[0];
    int status;
    int ans;
    while(list!=NULL)
    {/*WNOHANG means parent does not wait for child to be terminated, it return immediatly*/
        /*check the condition*/
        ans=waitpid(list->pid,&status,WNOHANG);
        // if(ans == 0)/*the process did not finish*/
        // if(ans == -1)/* if the process already terminated */
        if(ans == list->pid || ans == -1) /*the child has exited*/
        {   
            updateProcessStatus(list,list->pid,TERMINATED);
            updateProcessStatus(list,ans,TERMINATED);
            printf("%-10d %-13s %s\n",list->pid,list->cmd->arguments[0],statusString(list->status));
            /*delete this process*/
            if(list == process_list[0])
            {
                process* list2=list->next;
                list->next=NULL;
                process_list[0]=list2;
                freeProcessList(list);
                list=list2;
                continue;
            }
            else
            {
                prev->next=list->next;
                list->next=NULL;
                freeProcessList(list);
                list=prev;
            }
        }
        
        
        prev=list;
        list=list->next;
    }
}

void handler(int sig)
{
    printf("\nCaught Signal: %s\n", strsignal(sig));
    pid_t pid = wait(0);
    updateProcessStatus(procs[0],pid,TERMINATED);
}

void freeProcessList(process* process_list)
{
    if(process_list==NULL) return;
    freeCmdLines(process_list->cmd);
    if(process_list->next!=NULL) freeProcessList(process_list->next);
    free(process_list);
}

void updateProcessStatus(process* process_list, int pid, int status)
{
    process* curr=process_list;
    while(curr!=NULL)
    {
        if(curr->pid==pid)
        {
            curr->status=status;
            break;
        }
        curr=curr->next;
    }
}

int SKW(cmdLine* cmd)
{
    if(cmd->argCount==2)
    {
        int pid=atoi(cmd->arguments[1]);
        
        if(strcmp(cmd->arguments[0],"kill")==0)
        {
            // printf("kill %d\n",pid);
           kill(pid,SIGINT);
           return 1;
        }else if (strcmp(cmd->arguments[0],"suspend")==0)
        {
            // printf("suspend %d\n",pid);
            kill(pid,SIGTSTP);
            updateProcessStatus(procs[0],pid,SUSPENDED);
            return 1;
        }else if(strcmp(cmd->arguments[0],"wake")==0)
        {
            //  printf("wake %d\n",pid);
            kill(pid,SIGCONT);
            updateProcessStatus(procs[0],pid,RUNNING);
            return 1;
        }
    }
    return 0;
}

char *searchVar(char* key)
{
    vars* tmp=variables;
    while(tmp!=NULL)
    {
        if(strcmp(key,tmp->key)==0)
        {
            return tmp->value;
        }
        tmp=tmp->next;
    }
    return NULL;
}