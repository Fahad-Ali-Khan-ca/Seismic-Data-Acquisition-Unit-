//DataAcquisition.h - Class declaration for the DataAcquisition unit
//
// History:
// 25-Apr-23  Fahad Khan       Created.
// 27-Apr-23  Fahad Khan       Added function definitions.

#ifndef _DATAACQUISITION_H
#define _DATAACQUISITION_H
#include <errno.h>
#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <queue>
#include <netinet/in.h>
#include "SeismicData.h"
#include <map>
#include <list>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

static void interruptHandler(int signum);

//should be in public but it's okay
void *recv_func(void *arg);
void *write_func(void *arg);

//===================================CLASS======================================
class DataAcquisition {
public:
    static DataAcquisition* instance;
    DataAcquisition();
    ~DataAcquisition();
    // recieve function
    void ReceiveFunction();
    // write function
    void WriteFunction();
    void run();
    void shutdown();        
private:
//=============DATA MEMBERS===============================
    // Shared memory
    key_t ShmKey;
    int ShmID;
    struct SeismicMemory *ShmPTR;
    //semaphores
    sem_t *sem_id1;

    // Socket 
    int sockfd;
    struct sockaddr_in server_addr;
    bool is_running = false;
    
    //signal
    struct sigaction action;

    // to store data from transducer    
    struct DataPacket {
    unsigned short packet_length;
    uint16_t packet_number;
    SeismicData data;
    };

    // to store subscriber info
    struct Subscriber {
        string username;
        struct sockaddr_in address;
        uint16_t port;
    };
 //======================================================
 
 //=================FUNCTIONS=============================
    //function to setup socket
    void setupSocket();        
    //function to authenticate datacenter
    void authenticator();     
    //function to intialize semaphores
    void semInitializer();
    //function to intialize shared memory
    int setupSharedMemory();  // CAN BE DONE IN RUN();
    //function to intialize signal handler
    void interruptInitializer();
    //function to intialize threads
    void threadInitializer();
    //function to intialize socket
    void socketInitializer();
    // function to add a subscriber
    void addSubscriber(const std::string& username, const sockaddr_in& client_addr);
    // function to remove a subscriber
    void removeSubscriber(const sockaddr_in& client_addr);
    // function to add rouge data centres into the rouge_centre list
    void updateRogueCenters(const std::string& username, const sockaddr_in& client_addr );
    


//======================CONTAINERS=================================
    // Queue and subscribers list
    std::queue<DataPacket> data_queue;
    std::vector<Subscriber> subscribers;
   // Container for rogue data centers
    std::vector<Subscriber> rogue_data_centers;

    // variables for authentication 
    const std::string PASSWORD = "Leaf";
    const int ROGUE_THRESHOLD = 3;
//==============================================================
    // Create threads
    pthread_t read_thread, write_thread;
    //mutex
    pthread_mutex_t lock_x;
};
#endif //_DATAACQUISITION_H