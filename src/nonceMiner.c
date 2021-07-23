#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h> // gcc-only library (TODO: offer cl.exe alternative?)
#include <stdarg.h>
#ifdef _WIN32 // Windows-unique preprocessor directives for compatiblity
    #ifdef _WIN32_WINNT
        #undef _WIN32_WINNT
    #endif
    #define _WIN32_WINNT 0x0600 // Necessary for GetTickCount64
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <winbase.h>
    // CPU info defines
    #define GET_DEFAULT_N_THREADS(ptr_n_threads) do{\
        SYSTEM_INFO sysinfo;\
        GetSystemInfo(&sysinfo);\
        *(ptr_n_threads) = sysinfo.dwNumberOfProcessors;\
    }while(0);
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
    // Socket error reporting defines
    #define SOCK_ERRNO WSAGetLastError()
    #define SPRINT_SOCK_ERRNO(buf, buf_size, errno) do{\
        int len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errno, 0, buf, buf_size, NULL);\
        buf[len-1] = 0;\
    }while(0)
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <sys/sysinfo.h>
    #include <netdb.h>
    #include <errno.h>
    #include <unistd.h>
    #include <pthread.h>
    // CPU info defines
    #define GET_DEFAULT_N_THREADS(ptr_n_threads) do{\
        *(ptr_n_threads) = get_nprocs();\
    }while(0);
    // Socket defines
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
    // Socket error reporting defines
    #define SOCK_ERRNO errno
    #define SPRINT_SOCK_ERRNO(buf, buf_size, errno) strcpy(buf, strerror(errno))
#endif

#include "mine_DUCO_S1.h"
#include "mine_xxhash.h"

struct _thread_resources{
    int thread_id;
    int hashrate;
    int opencl_thread;
};

enum Intensity {LOW, MEDIUM, NET, EXTREME};

MUTEX_T count_lock; // Protects access to shares counters
check_nonce_ctx opencl_ctx;
int server_is_online = 1;
int using_xxhash = 0;
int shared_accepted = 0, shared_rejected = 0;
int job_request_len;
char job_request[256];
char server_address[256] = "149.91.88.18"; // Default server should be pulse-pool-1
char server_port[16] = "6000";
char username[128];
char identifier[128] = ""; // Default value should be empty string

// Prints log as formatted along with timestamp, newline, and four-letter code
void print_formatted_log(const char* code, const char* format, ...){
    time_t ts_epoch = time(NULL);
    struct tm *ts_clock = localtime(&ts_epoch);
    printf("%02d:%02d:%02d", ts_clock->tm_hour, ts_clock->tm_min, ts_clock->tm_sec);
    printf(" [%s] ", code);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
}

void print_help(){
    puts("nonceMiner v2.0.0 by colonelwatch");
    puts("A miner applying hash midstate caching and other optimzations to the duinocoin project");
    puts("Typical usage: nonceMiner -u <your username here> [OPTIONS]");
    puts("Options:");
    puts("  -h    Print the help message");
    puts("  -a    Specify the hash algorithm {DUCO_S1, xxhash}");
    puts("  -i    Job difficulty/intensity {LOW, MEDIUM, NET, EXTREME}");
    puts("  -o    Node URL of the format <host>:<port>");
    puts("  -u    Username for mining");
    puts("  -t    Number of threads");
    puts("  -g    Spawn one additional OpenCL (GPU) thread");
}

void* mining_routine(void* arg){
    int len;
    char buf[256], thread_code[16];
    TIMESTAMP_T t1, t0;
    struct _thread_resources *shared_data = arg;
    if(shared_data->opencl_thread)
        sprintf(thread_code, "gpu0");
    else
        sprintf(thread_code, "cpu%d", shared_data->thread_id);
    while(1){
        unsigned int soc = socket(PF_INET, SOCK_STREAM, 0);
        SET_TIMEOUT(soc, 16);

        // Resolves server address and port then connects
        struct addrinfo *dns_result;
        len = getaddrinfo((const char*)server_address, (const char*)server_port, NULL, &dns_result);
        if(len != 0){ // Custom exit-on-failure code for DNS resolution
            print_formatted_log(thread_code, "Error resolving server address: %s", gai_strerror(len)); 
            goto on_error;
        }
        len = connect(soc, dns_result->ai_addr, dns_result->ai_addrlen);
        if(len == -1){ // Boilerplate exit-on-failure code
            SPRINT_SOCK_ERRNO(buf, sizeof(buf), SOCK_ERRNO); // Reusing buf to print error message
            print_formatted_log(thread_code, "Error opening connection: %s", buf);
            goto on_error;
        }
        freeaddrinfo(dns_result);

        // Receives server version
        len = recv(soc, buf, 100, 0);
        if(len == -1){
            SPRINT_SOCK_ERRNO(buf, sizeof(buf), SOCK_ERRNO);
            print_formatted_log(thread_code, "Error connecting: %s", buf);
            goto on_error;
        }
        else if(len == 0){ // Boilerplate exit-on-disconnect code
            print_formatted_log(thread_code, "Error connecting: server closed gracefully");
            goto on_error;
        }
        buf[len] = 0;
        print_formatted_log(thread_code, "Connected to %s:%s with response: %s", server_address, server_port, buf);

        GET_TIME(&t0);

        while(1){
            // Sends job request
            len = send(soc, job_request, job_request_len, 0);
            if(len == -1){
                SPRINT_SOCK_ERRNO(buf, sizeof(buf), SOCK_ERRNO);
                print_formatted_log(thread_code, "Error sending job request: %s", buf);
                goto on_error;
            }

            // Receives job string
            len = recv(soc, buf, 128, 0);
            if(len == -1){
                SPRINT_SOCK_ERRNO(buf, sizeof(buf), SOCK_ERRNO);
                print_formatted_log(thread_code, "Error receiving job: %s", buf);
                goto on_error;
            }
            else if(len == 0){
                print_formatted_log(thread_code, "Error receiving job: server closed gracefully");
                goto on_error;
            }
            buf[len] = 0;

            int diff;
            long nonce;
            if(using_xxhash){
                if(buf[40] == ','){ // If the prefix is a SHA1 hex digest (40 chars long)...
                    diff = atoi((const char*) &buf[58]);
                    nonce = mine_xxhash(
                        (const unsigned char*) &buf[0],
                        40,
                        (const unsigned char*) &buf[41],
                        diff
                    );
                }
                else{ // Else the prefix is probably an XXHASH hex digest (16 chars long)...
                    diff = atoi((const char*) &buf[34]);
                    nonce = mine_xxhash(
                        (const unsigned char*) &buf[0],
                        16,
                        (const unsigned char*) &buf[17],
                        diff
                    );
                }
            }
            else{
                if(buf[40] == ','){ // If the prefix is a SHA1 hex digest (40 chars long)...
                    diff = atoi((const char*) &buf[82]);
                    print_formatted_log(thread_code, "New job from %s with difficulty %d", server_address, diff);
                    if(shared_data->opencl_thread){ // If this is an OpenCL thread...
                            // Then use the OpenCL path (not availible for xxhash prefixes yet)
                            nonce = mine_DUCO_S1_OpenCL(
                            (const unsigned char*) &buf[0],
                            40,
                            (const unsigned char*) &buf[41],
                            diff, 
                            &opencl_ctx
                        );
                    }
                    else{
                        nonce = mine_DUCO_S1(
                            (const unsigned char*) &buf[0],
                            40,
                            (const unsigned char*) &buf[41],
                            diff
                        );
                    }
                }
                else{ // Else the prefix is probably an XXHASH hex digest (16 chars long)...
                    diff = atoi((const char*) &buf[58]);
                    print_formatted_log(thread_code, "New job from %s with difficulty %d", server_address, diff);
                    nonce = mine_DUCO_S1(
                        (const unsigned char*) &buf[0],
                        16,
                        (const unsigned char*) &buf[17],
                        diff
                    );
                }
            }
            
            // Calculates hashrate to report back to server
            GET_TIME(&t1);
            int tdelta_ms = DIFF_TIME_MS(&t1, &t0);
            int local_hashrate = nonce/tdelta_ms*1000;
            t0 = t1;

            // Generates and sends result string
            if(shared_data->opencl_thread)
                len = sprintf(buf, "%ld,%d,nonceMiner v2.0.0,%s OpenCL\n", nonce, local_hashrate, identifier);
            else
                len = sprintf(buf, "%ld,%d,nonceMiner v2.0.0,%s\n", nonce, local_hashrate, identifier);
            len = send(soc, buf, len, 0);
            if(len == -1){
                SPRINT_SOCK_ERRNO(buf, sizeof(buf), SOCK_ERRNO);
                print_formatted_log(thread_code, "Error sending result: %s", buf);
                goto on_error;
            }

            // Receives response and parses it
            len = recv(soc, buf, 128, 0); // May take up to 10 seconds as of server v2.2!
            if(len == -1){
                SPRINT_SOCK_ERRNO(buf, sizeof(buf), SOCK_ERRNO);
                print_formatted_log(thread_code, "Error receiving feedback: %s", buf);
                goto on_error;
            }
            else if(len == 0){ 
                print_formatted_log(thread_code, "Error receiving feedback: server closed gracefully");
                goto on_error;
            }
            buf[len] = 0;
            int local_accepted_share = (strcmp(buf, "GOOD\n") == 0 || strcmp(buf, "BLOCK\n") == 0);

            if(local_accepted_share)
                print_formatted_log(thread_code, "Share accepted, work-time %d.%d s", tdelta_ms/1000, tdelta_ms%1000);
            else
                print_formatted_log(thread_code, "Share rejected, work-time %d.%d s", tdelta_ms/1000, tdelta_ms%1000);

            // Mutex section for updating shared statistics
            MUTEX_LOCK(&count_lock);
            shared_data->hashrate = local_hashrate;
            if(local_accepted_share) shared_accepted++;
            else shared_rejected++;
            MUTEX_UNLOCK(&count_lock);
        }

        on_error:
        CLOSE(soc);
        shared_data->hashrate = 0; // Zero out hashrate since the thread is not active
        SLEEP(2);
        print_formatted_log(thread_code, "Restarted due to an unexpected error", shared_data->thread_id);
    }
}

// Tests server connection by receiving server version with a timeout on the transaction
void* ping_routine(void *arg){
    int len;
    char buf[128];
    while(1){
        unsigned int soc = socket(PF_INET, SOCK_STREAM, 0);
        SET_TIMEOUT(soc, 16);

        // Resolves master server address and port then connects
        struct addrinfo *dns_result;
        len = getaddrinfo("server.duinocoin.com", "2814", NULL, &dns_result);
        if(len != 0) goto on_error; // getaddrinfo() returns 0 on success
        len = connect(soc, dns_result->ai_addr, dns_result->ai_addrlen);
        if(len == -1) goto on_error;
        freeaddrinfo(dns_result);

        len = recv(soc, buf, 100, 0);
        if(len == -1 || len == 0) goto on_error;
        buf[len] = 0;

        on_error:
        CLOSE(soc);
        // Updates server_is_online according to whether recv succeeded
        if(len == -1 || len == 0){
            print_formatted_log("ping", "Warning pinging master server: Timed out or failed");
            server_is_online = 0;
        }
        else server_is_online = 1;

        SLEEP(16);
    }
}

int main(int argc, char **argv){
    INIT_WINSOCK();

    int n_threads, using_OpenCL = 0;
    GET_DEFAULT_N_THREADS(&n_threads);
    enum Intensity diff = EXTREME;
    int opt;
    opterr = 0; // Disables default getopt error messages

    while((opt = getopt(argc, argv, "ha:i:o:u:w:t:g")) != -1){
        switch(opt){
            case 'h':
                print_help();
                return 0;
            case 'a':
                if(strcmp(optarg, "DUCO_S1") == 0) using_xxhash = 0;
                else if(strcmp(optarg, "xxhash") == 0) using_xxhash = 1;
                else{
                    fprintf(stderr, "Option -a requires an argument from the set {DUCO_S1, xxhash}");
                    return 1;
                }
                break;
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
            case 'g':
                using_OpenCL = 1;
                break;
            case '?':
                if(optopt == 'a' || optopt == 'i' || optopt == 'o' || optopt == 'u' || optopt == 'w' || optopt == 't')
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
    if(using_xxhash)
        job_request_len = sprintf(job_request, "JOBXX,%s\n", username);
    else
        job_request_len = sprintf(job_request, "JOB,%s,%s\n", username, diff_string);

    printf("Initializing nonceMiner v2.0.0...\n");
    printf("Configured with username '%s', ", username);
    printf("identifier '%s', ", identifier);
    printf("difficulty '%s', ", diff_string);
    printf("and %d thread(s).\n", n_threads);
    if(using_OpenCL){
        printf("OpenCL flag detected, configuring one additional GPU thread...\n");
        char *filenames[3] = {
            "OpenCL/buffer_structs_template.cl",
            "OpenCL/sha1.cl",
            "OpenCL/duco_s1.cl"
        };
        init_OpenCL();
        build_OpenCL_source(filenames, 3);
        build_check_nonce_kernel(&opencl_ctx, 65536);
    }
    if(using_xxhash)
        printf("Running in xxhash mode. WARNING: Per-thread hashrates over 0.9 MH/s may be rejected.\n");
    printf("Starting threads...\n");

    struct _thread_resources *thread_data_arr = calloc(n_threads+using_OpenCL, sizeof(struct _thread_resources));
    THREAD_T *mining_threads = malloc((n_threads+using_OpenCL)*sizeof(THREAD_T));
    MUTEX_CREATE(&count_lock);
    for(int i = 0; i < n_threads; i++){
        thread_data_arr[i].thread_id = i;
        thread_data_arr[i].opencl_thread = 0;
        THREAD_CREATE(&mining_threads[i], mining_routine, &thread_data_arr[i]);
        SLEEP(1);
    }
    if(using_OpenCL){
        thread_data_arr[n_threads].thread_id = n_threads;
        thread_data_arr[n_threads].opencl_thread = 1;
        THREAD_CREATE(&mining_threads[n_threads], mining_routine, &thread_data_arr[n_threads]);
        SLEEP(1);
    }
    
    THREAD_T ping_thread;
    THREAD_CREATE(&ping_thread, ping_routine, NULL);
    
    // Zeroes out reported work done while starting up threads
    MUTEX_LOCK(&count_lock);
    shared_accepted = 0;
    shared_rejected = 0;
    MUTEX_UNLOCK(&count_lock);

    while(1){
        SLEEP(10);
        
        int total_hashrate = 0;
        for(int i = 0; i < n_threads+using_OpenCL; i++) total_hashrate += thread_data_arr[i].hashrate;
        float megahash = (float)total_hashrate/1000000;

        MUTEX_LOCK(&count_lock);
        int accepted_copy = shared_accepted; // Takes copies to keep mutex section short
        int rejected_copy = shared_rejected;
        shared_accepted = 0;
        shared_rejected = 0;
        MUTEX_UNLOCK(&count_lock);

        printf("\n");
        print_formatted_log("rprt", "Hashrate: %.2f MH/s, Accepted %d, Rejected %d", megahash, accepted_copy, rejected_copy);
        printf("\n");
    }

    return 0;
}