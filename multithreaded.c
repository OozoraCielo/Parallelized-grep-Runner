#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <semaphore.h>

pthread_t tid[8]; //create id for threads
int letin[8]; //at start and after every enqueue, let threads enter the while loop
int sleeping[8]; //to see if the thread is sleeping
int currsleep=0; //to know the number of threads sleeping

pthread_mutex_t lock; //for locking critical sections
sem_t queuelock; //for forcing a worker sleep when queue is empty
int fin = 0; //if 1, every worker should wake up


//for using popen - to use grep command
FILE *popen(const char *command, const char *mode);
int pclose(FILE *stream);

//declaring the queue
char *queue[100000];
int Rear = - 1;
int Front = - 1;

//for the multithreaded version
struct args {
    char *exe;
    int ID;
    char *root;
    char *search;
    int N;
};


//enqueueing in the queue
void enqueue(char *insert_item)
{
    if (Rear == 100000 - 1)
       printf("Overflow \n");
    else
    {
        if (Front == - 1)
      
        Front = 0;
        Rear = Rear + 1;
        strcpy(queue[Rear],insert_item);
    }
} 

//dequeueing in the queue
void dequeue()
{
    if (Front == - 1 || Front > Rear)
    {
        printf("Underflow \n");
        return ;
    }
    else
    {
        Front = Front + 1;
    }
} 

//just for checking the queue
void show()
{
    
    if (Front == - 1)
        printf("Empty Queue \n");
    else
    {
        printf("Queue: \n");
        for (int i = Front; i <= Rear; i++)
            printf("%s ", queue[i]);
        printf("\n");
    }
} 


void *work(struct args *arg){
    DIR *dir;
    struct dirent *dp;

    while (Front<=Rear || letin[arg->ID] == 1){ //stop when there are nothing to do left; after every enqueue, threads can come again
        pthread_mutex_lock(&lock);
        sleeping[arg->ID] = 1; //the current thread will sleep
        currsleep++;
        pthread_mutex_unlock(&lock);

        if (currsleep==((struct args*)arg)->N)sem_post(&queuelock); //force a thread to wake up
        
        else if (currsleep<((struct args*)arg)->N) sem_wait(&queuelock); //makes the worker sleep if queue is empty
        
        pthread_mutex_lock(&lock);
        sleeping[arg->ID]==0; //the current thread wakes up
        currsleep--;
        pthread_mutex_unlock(&lock);

        if (fin == 1) break;//there is nothing to do left


        char current_file[300];
        pthread_mutex_lock(&lock);
        letin[arg->ID]=0;
        strcpy(current_file,queue[Front]); //get the first item/directory in the queue
        dequeue();
        printf("[%d] DIR %s\n",arg->ID,current_file);
        pthread_mutex_unlock(&lock);

        dir=opendir(current_file);
            

        while ((dp = readdir(dir)) != NULL) {
                
            if ((strcmp(dp->d_name,"."))==0 || (strcmp(dp->d_name,".."))==0){ //ignore these files
            }
            else{
                char checkfile[300];
                strcpy(checkfile, current_file);
                strcat(checkfile, "/");
                strcat(checkfile, dp->d_name); //the true current directory that needs to be checked

                    
                if (dp->d_type == DT_DIR){ //if it is a directory
                    char deepcheckfile[300];
                    strcpy(deepcheckfile,checkfile); //copy will be enqueued to the queue

                    pthread_mutex_lock(&lock);
                    enqueue(deepcheckfile);

                        
                    for (int j=0; j<8; j++){ //after an enqueue, refresh letin to let the threads come in again
                        letin[j]=1;
                    }
                        
                    printf("[%d] ENQUEUE %s\n",arg->ID,checkfile);
                    sem_post(&queuelock);
                        
                    pthread_mutex_unlock(&lock);
                        
                }
                else if(dp->d_type == DT_REG){ //if it is a regular file
                    char command[100000];
                        
                    strcpy(command, "grep -c \"");
                    strcat(command, ((struct args*)arg)->search);
                    strcat(command, "\" \"");
                    strcat(command, checkfile); //make the grep command
                    strcat(command, "\"");


                    FILE *grep;
                    grep= popen(command, "r"); //execute the grep command
                    if (grep == NULL) {
                        perror("popen");
                        exit(EXIT_FAILURE);
                    }

                    char grep_value[1024];
                    fgets(grep_value, sizeof(grep_value), grep); //get the output of grep
                    int Grep_value = atoi(grep_value);
                    pclose(grep);

                        
                    if (Grep_value!=0){ //something has been found
                        printf("[%d] PRESENT %s\n",arg->ID,checkfile);
                    }
                    else if(Grep_value==0){ //nothing has been found
                        printf("[%d] ABSENT %s\n",arg->ID,checkfile);
                    }
                }
            }
            
        }
        closedir(dir); //close the dir after using it
        
    }
    pthread_mutex_lock(&lock); //there is nothing to do left
    fin = 1;
    pthread_mutex_unlock(&lock);
    sem_post(&queuelock); //wakes up every sleeping process and force them to end
}


void main(int argc, char* argv[]){
    //allocating space for the queue
    for (int j=0; j<100000; j++){
        queue[j]=malloc(300);
    }

    for (int j=0; j<8; j++){
        letin[j]=1;
        sleeping[j]=0;
    }


    sem_init(&queuelock, 0, 1); //initializing the semaphore

    char *executable=argv[0];
    char *Number=argv[1];
    char *rootpath=argv[2];
    char *searchfor=argv[3];

    int N = atoi(Number);
    int slash = argv[2][0]; //get the first character of argv[2]

    char cwd[300]; //declare cwd or current directory

    if (slash == 47){ //if the first character of the rootpath is "/"
        strcpy(cwd, rootpath); //use it as the rootpath      
    }
    else{
        getcwd(cwd, sizeof(cwd)); //get the current directory
        strcat(cwd, "/");
        strcat(cwd, argv[2]); //add the given rootpath to it
    }

    enqueue(cwd);
    
    struct args arg[N];
    for (int i=0;i<N;i++){
        arg[i].exe=executable;
        arg[i].ID=i;
        arg[i].root=rootpath;
        arg[i].search=searchfor;
        arg[i].N=N;
        pthread_create(&tid[i], NULL, (void *) work, &arg[i]);
    }



    for (int i=0;i<N;i++){
        pthread_join(tid[i], NULL);
    }

    for (int k = 0; k<100000; k++){
        free(queue[k]);
    }

    pthread_mutex_destroy(&lock);

    return;
}

