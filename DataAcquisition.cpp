//DataAcuisition.cpp   ---- Fahad Ali Khan, Mohammad Aeraf, Inderpreet
// includes all function declaration and thread creation 
//
// History:
// 25-Apr-23  Fahad Khan       Created.
// 27-Apr-23  Fahad Khan       Added function definitions.
#include "DataAcquisition.h"
#include <sstream>      // for std::stringstream

DataAcquisition* DataAcquisition::instance=nullptr;

DataAcquisition::~DataAcquisition() {
    // Clean up
    shmdt((void *) ShmPTR);
    shmctl(ShmID, IPC_RMID, NULL);
    //close the socket
    close(sockfd);
    //close the semaphore
    sem_close(sem_id1);
    sem_unlink(SEMNAME);
    cout<<"DataAcquisition::~DataAcquisition: Done"<<endl;
}

DataAcquisition::DataAcquisition() {
    is_running=true;
    ShmPTR=nullptr;
    DataAcquisition::instance=this;
}

//intialize the signal
void DataAcquisition::interruptInitializer() {
    action.sa_handler = interruptHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
}

//signal handler
static void interruptHandler(int signum)
{
    cout << "DataAcquisition::interruptHandler: Interrupt signal (" << signum << ") received.\n";
    switch(signum) {
        case SIGINT:
            DataAcquisition::instance->shutdown();
            break;
    }
}

void DataAcquisition::shutdown() {
    cout<<"DataAcquisition::shutdown:"<<endl;
    is_running=false;
}

//setup the shared memory
int DataAcquisition::setupSharedMemory() {
    ShmKey = ftok(MEMNAME, 65);
    if (ShmKey == -1) {
        strerror(errno);

    }
    ShmID = shmget(ShmKey, sizeof(struct SeismicMemory), IPC_CREAT | 0666);
    if (ShmID < 0) {
        cout<<"DataAcquisition Unit: shmget() error"<<endl;
        cout<<strerror(errno)<<endl;
        return -1;
    }
    ShmPTR = (struct SeismicMemory *) shmat(ShmID, NULL, 0);
    if (ShmPTR == (void *)-1) {
        cout<<"DataAcquisition Unit: shmat() error"<<endl;
        cout<<strerror(errno)<<endl;
        return -1;
    }

    return 0;
}

//setup semaphores
void DataAcquisition::semInitializer() {
    sem_id1 = sem_open(SEMNAME, O_CREAT, SEM_PERMS, 0);
    if (sem_id1 == SEM_FAILED) {
        std::cerr << "Error: sem_open() failed" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void DataAcquisition::setupSocket() {
    sockfd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0) {
        std::cerr << "Error: socket() failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1153);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        std::cerr << "Error: setsockopt() failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: bind() failed" << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Add subscriber to the list of subscribers
void DataAcquisition::addSubscriber(const std::string& username, const sockaddr_in& client_addr) {
    Subscriber new_subscriber;
    new_subscriber.username = username;
    new_subscriber.address = client_addr;
    new_subscriber.port = ntohs(client_addr.sin_port);
    subscribers.push_back(new_subscriber);
}

// Remove subscriber from the list of subscribers
void DataAcquisition::removeSubscriber(const sockaddr_in& client_addr) {
    auto it = std::find_if(subscribers.begin(), subscribers.end(),
                 [&client_addr](const Subscriber &s) {
                     return s.address.sin_family == client_addr.sin_family
                         && s.address.sin_port == client_addr.sin_port
                         && s.address.sin_addr.s_addr == client_addr.sin_addr.s_addr;
                 });

    if (it != subscribers.end()) {
        subscribers.erase(it);
    }
}


void DataAcquisition:: updateRogueCenters(const std::string& username, const sockaddr_in& client_addr ){
    //add the given username to the RogueCentres List
    Subscriber new_subscriber;
    new_subscriber.username = username;
    new_subscriber.address = client_addr;
    new_subscriber.port = ntohs(client_addr.sin_port);
    rogue_data_centers.push_back(new_subscriber);    
}

// main function for the thread that reads data from the from the shared memory
void DataAcquisition::run() {

    //signal handler
    interruptInitializer();

    // Setup shared memory
    setupSharedMemory();

    // Setup semaphores
    semInitializer();

    // Setup socket
    setupSocket();

    // Create threads
    pthread_create(&read_thread, NULL, recv_func, (void *) this);
    pthread_create(&write_thread, NULL, write_func, (void *) this);
    
    is_running=true;

    // Data acquisition loop
    int seismicDataIndex = 0;
    while (is_running) {
        // Wait for the semaphore to be released
        sem_wait(sem_id1);
        // Check if the data is ready to be read
        if (ShmPTR->seismicData[seismicDataIndex].status == WRITTEN) {
            // Create a new data packet and populate it with shared memory data
            DataPacket packet;
            packet.packet_number = ShmPTR->packetNo;
            packet.packet_length = ShmPTR->seismicData[seismicDataIndex].packetLen;
            packet.data=ShmPTR->seismicData[seismicDataIndex];
            // Mark the data as read in shared memory
            ShmPTR->seismicData[seismicDataIndex].status = READ;
            // Release the semaphore
            sem_post(sem_id1);

            // Add the packet to the queue, lock the mutex to prevent data race
            pthread_mutex_lock(&lock_x);
            data_queue.push(packet);
            pthread_mutex_unlock(&lock_x);

            // Increment the index and reset if needed
            seismicDataIndex++;
            if (seismicDataIndex >= NUM_DATA) {
                seismicDataIndex = 0;
            }
        } else {
            // If the data is not ready, release the semaphore
            sem_post(sem_id1);
        }

        // Sleep for a short period to prevent high CPU usage
        sleep(1);
    }

    cout<<"DataAcquisition Unit: Shutting down..."<<endl;
    // Wait for threads to finish
    pthread_join(read_thread, NULL);
    pthread_join(write_thread, NULL);
    // Close the socket
    close(sockfd);
    // Close the semaphore
    sem_close(sem_id1);
    // Unlink the semaphore
    sem_unlink(SEMNAME);
    // Detach the shared memory
    shmdt((void *) ShmPTR);
    // Delete the shared memory
    shmctl(ShmID, IPC_RMID, NULL);
    // Destroy the mutex
    pthread_mutex_destroy(&lock_x);
    // Exit the program
    exit(EXIT_SUCCESS);
}

// Read thread function
void *recv_func(void *arg)
{
    DataAcquisition *data_acquisition = (DataAcquisition *) arg;
    data_acquisition->ReceiveFunction();
    pthread_exit(NULL);
}

// Write thread function
void *write_func(void *arg)
{
    DataAcquisition *data_acquisition = (DataAcquisition *) arg;
    data_acquisition->WriteFunction();
    pthread_exit(NULL);
}

// Receive data from the data centres
void DataAcquisition::ReceiveFunction() {
    int ret;
    char buffer[BUF_LEN];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    std::map<std::string, int> ip_connection_count;
    bool rogue;
    while (is_running) {
        //flush the buffer
        memset(buffer, 0, BUF_LEN);
        rogue =false;
        // Receive data from the socket  
        ret = recvfrom(sockfd, buffer, BUF_LEN, 0, (struct sockaddr *) &client_addr, &client_addr_len);
        if(ret<0) {
            sleep(0.1);
        }else{
            uint16_t client_port = ntohs(client_addr.sin_port);

            // Get the IP address of the client
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            std::string ip_address = std::string(client_ip);
            // Create a unique string to represent the combination of IP address and port
            std::string ip_and_port = ip_address + ":" + std::to_string(client_port);
            // Increment the connection count for the IP address and port

            for (auto it = rogue_data_centers.begin(); it != rogue_data_centers.end(); ++it) {
                if (it->port==client_port) {
                    rogue = true;
                    break;
                }
            }
            // I could've made a function here but this worked after a long time of work and I don't want to touch it
            if(!rogue) 
            {
                char *save_ptr;
                char token[256];
                char username[256];
                char actionString[256];
                char password[256] = "";
                int data_center_number;

                strncpy(actionString, strtok_r(buffer, ",", &save_ptr), sizeof(action) - 1);
                strncpy(username, strtok_r(NULL, ",", &save_ptr), sizeof(username) - 1);
                // sscanf(username, "DataCenter%d", &data_center_number); 
                if (strcmp(actionString, "Subscribe") == 0) {
                char *password_token = strtok_r(NULL, ",", &save_ptr);
                if (password_token) {
                    strncpy(password, password_token, sizeof(password) - 1);
                } else {
                        password[0] = '\0';
                }
                } else {
                    password[0] = '\0';
                }

                ip_connection_count[ip_and_port]++;

                if (strcmp(actionString, "Subscribe") != 0 && strcmp(actionString, "Cancel") !=0) {
                    cout<<"DataAcquisition: unkown command "<< actionString <<endl;
                } else if (strcmp(actionString, "Subscribe")== 0 && strcmp(password, "Leaf")== 0) {
                    bool isSubscribed = false;
                    //check if the data center is already subscribed
                    for (auto it = subscribers.begin(); it != subscribers.end(); ++it) {
                        if (it->port == client_port) {
                            isSubscribed = true;
                            cout << it->username << " has already Subscribed!!" << endl;
                        break;
                        }
                    }   
                    if(!isSubscribed && strcmp(password, "Leaf")== 0){            
                        const char data[] = "Subscribed";
                        ret = sendto(sockfd, data, sizeof(data), 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
                        pthread_mutex_lock(&lock_x);
                    addSubscriber(username, client_addr);
                    pthread_mutex_unlock(&lock_x);
                    cout<< username << " Subscribed!"<<endl;
                    }
                    } else if (strcmp(actionString, "Cancel")== 0) {
                        pthread_mutex_lock(&lock_x);
                        removeSubscriber(client_addr);
                        pthread_mutex_unlock(&lock_x);
                        cout<< username << " Cancelled"<<endl;
                        //connection count now zero
                        ip_connection_count[ip_and_port] = 0;
                    } 
                    if (ip_connection_count[ip_and_port] > ROGUE_THRESHOLD) {
                        cout << "Adding " << ip_and_port << " to the rogue client list." << std::endl;
                        pthread_mutex_lock(&lock_x);
                        updateRogueCenters(username, client_addr);
                        removeSubscriber(client_addr);
                        pthread_mutex_unlock(&lock_x);
                       ip_connection_count[ip_and_port] = 0;       
                    }
            }
        }
    }
}

void DataAcquisition::WriteFunction() {
    int ret;
    while (is_running) {
        // If there is no data in the queue, sleep for a short period
        if (data_queue.empty()) {
            usleep(10000); // Sleep for 10 milliseconds
            continue;
        }
            // Lock and unlock when done the mutex, to prevent data race
            pthread_mutex_lock(&lock_x);
            // Get the data packet from the queue
            DataPacket packet = data_queue.front();
            data_queue.pop();
            pthread_mutex_unlock(&lock_x);
            //we need to store the first two bytes of the packet as the packet number because 2096 cannot be represented in a single byte
            
            // Create a buffer to store the data packet
            // const int len=packet.data.packetLen+3; it should be like this but I chose BUF_LEN to simulate 
            // example screenshot
            char buf[BUF_LEN] ="";
            //this stores the first byte as packet number and the next two bytes as packet length
            // it should be vice versa but I chose this to simulate example screenshot
            buf[0] = packet.packet_number & 0xFF;
            buf[1] = (packet.packet_length >> 8) & 0XFF;
            buf[2] = packet.packet_length & 0xFF;
            memcpy(buf + 3, packet.data.data, packet.packet_length);
            // Iterate through the list of subscribers
            for (const Subscriber& subscriber : subscribers) {
            ret = sendto(sockfd, buf, BUF_LEN, 0,(struct sockaddr*)&subscriber.address, sizeof(subscriber.address));

            if (ret < 0) {
                cerr << "Error: sendto() failed for subscriber " << subscriber.username
                          << ". Error message from strerror: " << strerror(errno) << std::endl;
             } else {
                    char subscriber_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(subscriber.address.sin_addr), subscriber_ip, INET_ADDRSTRLEN);
                    std::string ip_address = std::string(subscriber_ip);
                    cout << "Send func: " << ip_address <<": "<< subscriber.port << std::endl;
                    //print data_queue size and subscriber list size
                    cout << "dataPacket.size():" << data_queue.size()<< "client.size():"<<subscribers.size() <<endl;
                }
            }
        
        // Sleep for a short period to prevent high CPU usage
       sleep(1); 
    }
}

