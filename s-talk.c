#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "list.h"
#include "send.h"
#include "receive.h"

#define NUM_THREADS 4

struct thread_data{
   char* hostname;
   char* localport; 
   char* remoteport;
};

List* sendList;
List* receiveList;

pthread_mutex_t send_mutex;
pthread_cond_t send_cv;
pthread_mutex_t receive_mutex;
pthread_cond_t receive_cv;

bool running;

/* On receipt of input, adds the input to the list of messages(sendList) that need to be sent to the remote s-talk client */
void* keyboardInput(void *threadarg) {
  char str[10000];
  running = true;
  while (true) {
    fgets(str, sizeof(str), stdin);
    if (strcmp(str, "!") == 10) {
        running = false;
        pthread_exit(NULL);
    }
    pthread_mutex_lock(&send_mutex);
    List_append(sendList, str);
    pthread_cond_signal(&send_cv);
    pthread_mutex_unlock(&send_mutex);
  }
}

/* Take message off the receiveList and output it to the screen */
void* screenOutput(void *threadarg) {
    while (running) {
        pthread_mutex_lock(&receive_mutex);
        pthread_cond_wait(&receive_cv, &receive_mutex);
        char* buf = List_trim(receiveList);
        printf("\"%s\"\n", buf);
        pthread_mutex_unlock(&receive_mutex);
    }
}

/* On receipt of input from the remote s-talk client, put the message onto the list of messages(receiveList) that need to be printed to the local screen */
void* UDPInput(void *threadarg) {
    struct thread_data *instanceInfo;
    instanceInfo = (struct thread_data *) threadarg;
    while (running) {
        char* buf = receiveMessage(instanceInfo->localport);
        pthread_mutex_lock(&receive_mutex);
        List_append(receiveList, buf);
        pthread_cond_signal(&receive_cv);
        pthread_mutex_unlock(&receive_mutex);
    }
    pthread_exit(NULL);
}

/* Take message off the sendList and send it over the network to the remote client */
void* UDPOutput(void *threadarg) {
    struct thread_data *instanceInfo;
    instanceInfo = (struct thread_data *) threadarg;
    while (running) {
        pthread_mutex_lock(&send_mutex);
        pthread_cond_wait(&send_cv, &send_mutex);
        char* str = List_trim(sendList);
        pthread_mutex_unlock(&send_mutex);
        sendMessage(instanceInfo->hostname, instanceInfo->remoteport, str);
    }
}

int main(int argc, char *argv[]) 
{   

    /* arg[0] = s-talk, arg[1] = localport, arg[2] = hostname of remote, arg[3] = remoteport */
    if (argc != 4) {
        fprintf(stderr,"usage: s-talk localport hostname remoteport\n");
        exit(1);
    }

    sendList = List_create();
    receiveList = List_create();

    struct thread_data instanceInfo;
    instanceInfo.hostname = argv[2];
    instanceInfo.localport = argv[1];
    instanceInfo.remoteport = argv[3];

    pthread_t threads[NUM_THREADS];

    /* Keyboard Input Process */
    if (pthread_create(&threads[0], NULL, keyboardInput, (void *) &instanceInfo) == 1) {
        fprintf(stderr, "keyboard input failed\n");
        exit(-1);
    }

    /* Screen Output Process */
    if (pthread_create(&threads[1], NULL, screenOutput, (void *) &instanceInfo) == 1) {
        fprintf(stderr, "screen output failed\n");
        exit(-1);
    }

    /* UDP Input Process */
    if (pthread_create(&threads[2], NULL, UDPInput, (void *) &instanceInfo) == 1) {
        fprintf(stderr, "UDP input failed\n");
        exit(-1);
    }

    /* UDP Output Process */
    if (pthread_create(&threads[3], NULL, UDPOutput, (void *) &instanceInfo) == 1) {
        fprintf(stderr, "UDP output failed\n");
        exit(-1);
    }

    pthread_exit(NULL);
}