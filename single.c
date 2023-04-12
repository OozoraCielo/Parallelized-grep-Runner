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




void main(int argc, char* argv[]){
    //allocating space for the queue
    for (int j=0; j<100000; j++){
        queue[j]=malloc(300);
    }

    //variables for the multithreaded version
    char *executable=argv[0];
    char *Number=argv[1];
    char *rootpath=argv[2];
    char *searchfor=argv[3];

    
    int N = atoi(Number); //convert argv[1] into int
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
    
    DIR *dir;
    struct dirent *dp;

    while (Front<=Rear){ //stop when queue is empty
        char current_file[300];
        strcpy(current_file,queue[Front]); //get the first item/directory in the queue

        dequeue();
        printf("[0] DIR %s\n",current_file);
        if ((dir = opendir(current_file)) != NULL){ //opendir that directory
            
        
            while ((dp = readdir(dir)) != NULL) {
                
                if ((strcmp(dp->d_name,"."))==0 || (strcmp(dp->d_name,".."))==0){ //ignore these files
                }
                else{
                    char checkfile[300];
                    strcpy(checkfile, current_file);
                    strcat(checkfile, "/");
                    strcat(checkfile, dp->d_name); //the true current directory that needs to be checked

                    //sprintf(checkfile,"%s/%s",current_file,dp->d_name);
                    
                    if (dp->d_type == DT_DIR){ //if it is a directory
                        char deepcheckfile[300];
                        strcpy(deepcheckfile,checkfile); //copy will be enqueued to the queue

                        enqueue(deepcheckfile);
                        printf("[0] ENQUEUE %s\n",checkfile);
                    }
                    else if(dp->d_type == DT_REG){ //if it is a regular file
                        char command[100000];
                        
                        strcpy(command, "grep -c \"");
                        strcat(command, argv[3]);
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
                            printf("[0] PRESENT %s\n",checkfile);
                        }
                        else if(Grep_value==0){ //nothing has been found
                            printf("[0] ABSENT %s\n",checkfile);
                        }
                    }
                }
            }
            closedir(dir); //close the dir after using it
            //show();
        }
        
    }
    //free the space taken by queue
    for (int k = 0; k<100000; k++){
        free(queue[k]);
    }

    return;
}