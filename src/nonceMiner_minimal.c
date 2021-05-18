/*
    Minimum code required to solve a DUCO-S1 job, with no error handling or multithreading
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

#include "mine_DUCO_S1.h"
#define USERNAME "revox"

int main(){
    INIT_WINSOCK();

    int len;
    char buf[128], job_request[256];
    int job_request_len = sprintf(job_request, "JOB,%s,EXTREME", USERNAME);

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr("51.15.127.80");
    server.sin_family = AF_INET;
    server.sin_port = htons(2811);
    unsigned int soc = socket(PF_INET, SOCK_STREAM, 0);

    len = connect(soc, (struct sockaddr *)&server, sizeof(server));

    len = recv(soc, buf, 3, 0);
    buf[len] = 0;

    while(1){
        send(soc, job_request, job_request_len, 0);

        len = recv(soc, buf, 128, 0);
        buf[len] = 0;

        int diff = atoi((const char*) &buf[82]);
        long nonce = mine_DUCO_S1_extend_cache(
            (const unsigned char*) &buf[0],
            (const unsigned char*) &buf[41],
            diff
        );

        len = sprintf(buf, "%ld\n", nonce);
        send(soc, buf, len, 0);

        len = recv(soc, buf, 128, 0); // May take up to 10 seconds as of server v2.2!
        buf[len] = 0;

        if(!strcmp(buf, "GOOD\n") || !strcmp(buf, "BLOCK\n"))
            printf("Accepted share %ld Difficulty %d\n", nonce, diff);
        else
            printf("Rejected share %ld Difficulty %d\n", nonce, diff);
    }

    return 0;
}