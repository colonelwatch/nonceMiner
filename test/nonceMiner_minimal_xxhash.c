/*
    Minimum code required to solve a xxhash job, with no error handling or multithreading
    and intended for educational/diagnostic purposes only.
    For extended use, please run the full version!
*/
#include <stdio.h>
#include <string.h>
#ifdef _WIN32 // Windows-unique preprocessor directives for compatiblity
    #include <winsock2.h>
    // Socket defines
    #define INIT_WINSOCK() do{\
        WSADATA wsa_data;\
        WSAStartup(MAKEWORD(1,1), &wsa_data);\
    }while(0)  // INIT_WINSOCK() must be called before using socket functions in Windows
    // Sleep defines
    #define SLEEP(seconds) Sleep((seconds)*1000)
#else
    // Socket defines
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define INIT_WINSOCK() // No-op
    // Sleep defines
    #define SLEEP(seconds) sleep(seconds)
#endif

#include "../src/mine_xxhash.h"
#define USERNAME "revox"

int main(){
    INIT_WINSOCK();

    printf("Running in xxhash mode. WARNING: Per-thread hashrates over 0.9 MH/s may be rejected.\n");

    int len;
    char buf[128], job_request[256];
    int job_request_len = sprintf(job_request, "JOBXX,%s\n", USERNAME);

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("149.91.88.18");
    server.sin_family = AF_INET;
    server.sin_port = htons(6000);
    unsigned int soc = socket(PF_INET, SOCK_STREAM, 0);

    len = connect(soc, (struct sockaddr *)&server, sizeof(server));

    // Receives the server version
    len = recv(soc, buf, 100, 0);
    buf[len] = 0;

    while(1){
        send(soc, job_request, job_request_len, 0); // Sends job request

        len = recv(soc, buf, 128, 0); // Receives job string
        buf[len] = 0;
        printf(buf);

        int diff;
        long nonce;
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

        // Generates and sends result string
        len = sprintf(buf, "%ld\n", nonce);
        send(soc, buf, len, 0);

        // Receives response
        len = recv(soc, buf, 128, 0); // May take up to 10 seconds as of server v2.2!
        buf[len] = 0;
        printf(buf);

        if(!strcmp(buf, "GOOD\n") || !strcmp(buf, "BLOCK\n"))
            printf("Accepted share %ld Difficulty %d\n", nonce, diff);
        else
            printf("Rejected share %ld Difficulty %d\n", nonce, diff);
    }

    return 0;
}