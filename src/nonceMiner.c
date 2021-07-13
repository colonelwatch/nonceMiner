#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#ifdef _WIN32 // Windows-unique preprocessor directives for compatiblity
    #ifdef _WIN32_WINNT
        #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT 0x0600 // Necessary for GetTickCount64
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    // Socket defines
    #define INIT_WINSOCK() do{\
        WSADATA wsa_data;\
        WSAStartup(MAKEWORD(1,1), &wsa_data);\
    }while(0)  // INIT_WINSOCK() must be called before using socket functions in Windows
    #define SET_TIMEOUT(soc, seconds) do{\
        unsigned long timeout = (seconds)*1000;\
        setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));\
        setsockopt(soc, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));\
    }while(0)
    #define CLOSE(soc) do{\
        shutdown(soc, SD_BOTH);\
        closesocket(soc);\
    }while(0)
    // Sleep defines
    #define SLEEP(seconds) Sleep((seconds)*1000)
    // Threading defines
    #define THREAD_T HANDLE // is type void*
    #define THREAD_CREATE(thread_ptr, func, arg_ptr) \
        *(thread_ptr) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) func, arg_ptr, 0, NULL)
    #define MUTEX_T CRITICAL_SECTION
    #define MUTEX_CREATE(mutex_ptr) InitializeCriticalSection(mutex_ptr)
    #define MUTEX_LOCK(mutex_ptr) EnterCriticalSection(mutex_ptr)
    #define MUTEX_UNLOCK(mutex_ptr) LeaveCriticalSection(mutex_ptr)
    // Timing defines
    #define TIMESTAMP_T long long
    #define GET_TIME(t_ptr) *(t_ptr) = GetTickCount64()
    #define DIFF_TIME_MS(t1_ptr, t0_ptr) *(t1_ptr)-*(t0_ptr)
#else
    // Socket defines
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <pthread.h>
    #define INIT_WINSOCK() // No-op
    #define SET_TIMEOUT(soc, seconds) do{\
        struct timeval timeout = {.tv_sec = seconds, .tv_usec = 0};\
        setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));\
        setsockopt(soc, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));\
    }while(0)
    #define CLOSE(soc) do{\
        shutdown(soc, SHUT_RDWR);\
        close(soc);\
    }while(0)
    // Sleep defines
    #define SLEEP(seconds) sleep(seconds)
    // Threading defines
    #define THREAD_T pthread_t
    #define THREAD_CREATE(thread_ptr, func, arg_ptr) pthread_create(thread_ptr, NULL, func, arg_ptr)
    #define MUTEX_T pthread_mutex_t
    #define MUTEX_CREATE(mutex_ptr) pthread_mutex_init(mutex_ptr, NULL)
    #define MUTEX_LOCK(mutex_ptr) pthread_mutex_lock(mutex_ptr)
    #define MUTEX_UNLOCK(mutex_ptr) pthread_mutex_unlock(mutex_ptr)
    // Timing defines
    #define TIMESTAMP_T struct timespec
    #define GET_TIME(t_ptr) clock_gettime(CLOCK_MONOTONIC, t_ptr)
    #define DIFF_TIME_MS(t1_ptr, t0_ptr) \
        ((t1_ptr)->tv_sec-(t0_ptr)->tv_sec)*1000+((t1_ptr)->tv_nsec-(t0_ptr)->tv_nsec)/1000000
#endif

#include "mine_DUCO_S1.h"

struct _thread_resources{
    int hashrate;
};

enum Intensity {LOW, MEDIUM, NET, EXTREME};

MUTEX_T count_lock; // Protects access to shares counters
int server_is_online = 1;
int accepted = 0, rejected = 0;
int job_request_len;
char job_request[256];
char server_address[256] = "149.91.88.18"; // Default server should be pulse-pool-1
char server_port[16] = "6000";
char username[128];
char identifier[128] = ""; // Default value should be empty string

struct option long_options[] = {
    {"intensity", required_argument, NULL, 'i'},
    {"url", required_argument, NULL, 'o'},
    {"user", required_argument, NULL, 'u'},
    {"worker", required_argument, NULL, 'w'},
    {"threads", required_argument, NULL, 't'}
};

void* mining_routine(void* arg){
    int len;
    char buf[256];
    TIMESTAMP_T t1, t0;
    struct _thread_resources *shared_data = arg;
    while(1){
        unsigned int soc = socket(PF_INET, SOCK_STREAM, 0);
        SET_TIMEOUT(soc, 16);

        // Resolves server address and port then connects
        struct addrinfo *dns_result;
        len = getaddrinfo((const char*)server_address, (const char*)server_port, NULL, &dns_result);
        if(len != 0) goto on_error;
        len = connect(soc, dns_result->ai_addr, dns_result->ai_addrlen);
        if(len == -1) goto on_error;
        freeaddrinfo(dns_result);

        // Receives server version
        len = recv(soc, buf, 100, 0);
        if(len == -1 || len == 0) goto on_error;
        buf[len] = 0;

        GET_TIME(&t0);

        while(1){
            // Sends job request
            len = send(soc, job_request, job_request_len, 0);
            if(len == -1) goto on_error; // Boilerplate exit-on-failure line

            // Receives job string
            len = recv(soc, buf, 128, 0);
            if(len == -1 || len == 0) goto on_error; // Adds exit-on-disconnect
            buf[len] = 0;

            int diff;
            long nonce;
            if(buf[40] == ','){ // If the prefix is a SHA1 hex digest (40 chars long)...
                diff = atoi((const char*) &buf[82]);
                nonce = mine_DUCO_S1(
                    (const unsigned char*) &buf[0],
                    40,
                    (const unsigned char*) &buf[41],
                    diff
                );
            }
            else{ // Else the prefix is probably an XXHASH hex digest (16 chars long)...
                diff = atoi((const char*) &buf[58]);
                nonce = mine_DUCO_S1(
                    (const unsigned char*) &buf[0],
                    16,
                    (const unsigned char*) &buf[17],
                    diff
                );
            }
            
            // Calculates hashrate to report back to server
            GET_TIME(&t1);
            int tdelta_ms = DIFF_TIME_MS(&t1, &t0);
            int local_hashrate = nonce/tdelta_ms*1000;
            t0 = t1;

            // Generates and sends result string
            len = sprintf(buf, "%ld,%d,nonceMiner v1.4.2,%s\n", nonce, local_hashrate, identifier);
            len = send(soc, buf, len, 0);
            if(len == -1) goto on_error;

            // Receives response
            len = recv(soc, buf, 128, 0); // May take up to 10 seconds as of server v2.2!
            if(len == -1 || len == 0) goto on_error;
            buf[len] = 0;

            // Mutex section for updating shared statistics
            MUTEX_LOCK(&count_lock);
            shared_data->hashrate = local_hashrate;
            if(!strcmp(buf, "GOOD\n") || !strcmp(buf, "BLOCK\n")) accepted++;
            else rejected++;
            MUTEX_UNLOCK(&count_lock);
        }

        on_error:
        CLOSE(soc);
        shared_data->hashrate = 0; // Zero out hashrate since the thread is not active
        SLEEP(2);
    }
}

// Tests server connection by receiving server version with a timeout on the transaction
void* ping_routine(void *arg){
    int len;
    char buf[128];
    while(1){
        struct sockaddr_in server;
        server.sin_addr.s_addr = inet_addr("51.15.127.80");
        server.sin_family = AF_INET;
        server.sin_port = htons(2814);
        unsigned int soc = socket(PF_INET, SOCK_STREAM, 0);
        
        SET_TIMEOUT(soc, 16);
        
        len = connect(soc, (struct sockaddr *)&server, sizeof(server));
        if(len == -1) goto on_error;

        len = recv(soc, buf, 100, 0);
        if(len == -1 || len == 0) goto on_error;
        buf[len] = 0;

        on_error:
        CLOSE(soc);
        // Updates server_is_online according to whether recv succeeded
        if(len == -1 || len == 0) server_is_online = 0;
        else server_is_online = 1;

        SLEEP(2);
    }
}

int main(int argc, char **argv){
    INIT_WINSOCK();

    int n_threads = -1;
    enum Intensity diff = EXTREME;
    int opt;

    opterr = 0; // Disables default getopt error messages

    while((opt = getopt_long(argc, argv, "i:o:u:w:t:", long_options, NULL)) != -1){
        switch(opt){
            case 'i':
                if(strcmp(optarg, "LOW") == 0) diff = LOW;
                else if(strcmp(optarg, "MEDIUM") == 0) diff = MEDIUM;
                else if(strcmp(optarg, "NET") == 0) diff = NET;
                else if(strcmp(optarg, "EXTREME") == 0) diff = EXTREME;
                else{
                    fprintf(stderr, "Option -i requires an argument from the set {LOW, MEDIUM, NET, EXTREME}");
                    return 1;
                }
                break;
            case 'o': ; // Empty statement preceding declaration is necessary
                char *start_ptr = optarg;
                if(strncmp(optarg, "tcp://", 6) == 0) start_ptr += 6; // For now, ignores tcp header if present
                if(sscanf((const char*)start_ptr, "%255[a-z0-9.]:%15[0-9]", server_address, server_port) != 2){
                    fprintf(stderr, "Option -o is not a valid address and port");
                    return 1;
                }
                break;
            case 'u':
                strcpy(username, optarg);
                break;
            case 'w':
                strcpy(identifier, optarg);
                break;
            case 't':
                if(sscanf(optarg, "%d", &n_threads) != 1 || n_threads <= 0){
                    fprintf(stderr, "Option -t requires a positive integer argument.\n");
                    return 1;
                }
                break;
            case '?':
                if(optopt == 'i' || optopt == 'o' || optopt == 'u' || optopt == 'w' || optopt == 't')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                return 1;
        }
    }

    if(strcmp(username, "") == 0){
        fprintf(stderr, "Missing username '-u'.\n");
        return 1;
    }
    if(n_threads == -1){
        fprintf(stderr, "Missing number of threads '-t'\n");
        return 1;
    }
    
    // Prepares diff_string and job_request using username and diff
    char diff_string[32];
    switch(diff){
        case LOW:
            strcpy(diff_string, "LOW");
            break;
        case MEDIUM:
            strcpy(diff_string, "MEDIUM");
            break;
        case NET:
            strcpy(diff_string, "NET");
            break;
        case EXTREME:
            strcpy(diff_string, "EXTREME");
            break;
    }
    job_request_len = sprintf(job_request, "JOB,%s,%s\n", username, diff_string);

    puts("Initializing nonceMiner v1.4.2...");
    printf("Configured with username '%s', identifier '%s', %d thread(s), and difficulty '%s'.\n", username, identifier, n_threads, diff_string);
    
    struct _thread_resources *thread_data_arr = calloc(n_threads, sizeof(struct _thread_resources));
    THREAD_T *mining_threads = malloc(n_threads*sizeof(THREAD_T));
    MUTEX_CREATE(&count_lock);
    puts("Starting threads...");
    for(int i = 0; i < n_threads; i++){
        THREAD_CREATE(&mining_threads[i], mining_routine, &thread_data_arr[i]);
        SLEEP(1);
    }
    THREAD_T ping_thread;
    THREAD_CREATE(&ping_thread, ping_routine, NULL);
    
    // Zeroes out reported work done while starting up threads
    MUTEX_LOCK(&count_lock);
    accepted = 0;
    rejected = 0;
    MUTEX_UNLOCK(&count_lock);

    while(1){
        SLEEP(2);
        
        int total_hashrate = 0;
        for(int i = 0; i < n_threads; i++) total_hashrate += thread_data_arr[i].hashrate;
        float megahash = (float)total_hashrate/1000000;

        MUTEX_LOCK(&count_lock);
        int accepted_copy = accepted; // Takes copies to keep mutex section short
        int rejected_copy = rejected;
        accepted = 0;
        rejected = 0;
        MUTEX_UNLOCK(&count_lock);

        time_t ts_epoch = time(NULL);
        struct tm *ts_clock = localtime(&ts_epoch);
        printf("%02d:%02d:%02d", ts_clock->tm_hour, ts_clock->tm_min, ts_clock->tm_sec);
        printf("  Hashrate: %.2f MH/s,", megahash);
        printf(" Accepted %d, Rejected %d", accepted_copy, rejected_copy);
        if(!server_is_online) printf(", server ping timeout\n");
        else printf("\n");
    }

    return 0;
}