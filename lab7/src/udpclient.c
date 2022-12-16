#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

//#define SERV_PORT 20001
//#define BUFSIZE 1024
#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int sockfd, n;
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  int SERV_PORT = -1;
  int BUFSIZE = -1;
  char IP[16] = {'\0'};

  while (1) {
    int current_optind = optind ? optind : 1;
    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"bufsize", required_argument, 0, 0},
                                      {"ip",required_argument,0,0},
                                      {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1)
        break;
    switch (c) {
        case 0: {
            switch (option_index) {
                case 0:
                    SERV_PORT = atoi(optarg);
                    if (!(SERV_PORT>0))
                        return 0;
                    break;
                case 1:
                    BUFSIZE = atoi(optarg);
                    if (!(BUFSIZE>0))
                        return 0;
                    break;
                case 2:
                    strcpy(IP, optarg);
                    if (!(strlen(IP)>0))
                        return 0;
                    break;
                default:
                    printf("Index %d is out of options\n", option_index);
            }
        } break;
        case '?':
            printf("Unknown argument\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }
  if (SERV_PORT == -1 || BUFSIZE == -1 || strlen(IP)==0) {
    fprintf(stderr, "Using: %s --port 20001 --bufsize 4 --ip 127.0.0.1\n", argv[0]);
    return 1;
  }

  char  recvline[BUFSIZE + 1];

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  if (inet_pton(AF_INET, IP, &servaddr.sin_addr) < 0) {
    perror("inet_pton problem ");
    exit(1);
  }

  servaddr.sin_port = htons(SERV_PORT);

  if (connect(sockfd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect problem ");
    exit(1);
  }

  //write(1, "Enter string: \n", 13);
   if (write(sockfd, &BUFSIZE, sizeof(int)) < 0) {
    perror("write");
    exit(1);
  }

  int response = 0;
  if (read(sockfd, &response, sizeof(int)) < 0) {
    fprintf(stderr, "Recieve failed\n");
    exit(1);
  }
  printf("PORT %u  \n", response);
 
  int  PORT = response;
  int fd;
  struct sockaddr_in servaddr1;

  memset(&servaddr1, 0, sizeof(servaddr1));
  servaddr1.sin_family = AF_INET;
  servaddr1.sin_port = htons(PORT);

  if (inet_pton(AF_INET, argv[1], &servaddr1.sin_addr) < 0) {
    perror("inet_pton problem");
    exit(1);
  }
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }
  n=BUFSIZE;
 
  for (int i = 1; i < n + 1; i++) {
    if (sendto(fd, &i, sizeof(i), 0, (struct sockaddr *)&servaddr1,
               sizeof(servaddr1)) == -1) {
      perror("sendto problem1");
      exit(1);
    }
  }
  
  int i = 0;
  if (sendto(fd, &i, sizeof(i), 0, (SADDR *)&servaddr1,
             sizeof(servaddr1)) == -1) {
    perror("sendto problem2");
    exit(1);
  }
  sleep(1);
  

    response = -1;
    int ad = 0;

    while(1) {
      if (read(sockfd, &response, sizeof(response)) < 0) {
        fprintf(stderr, "Recieve failed\n");
        exit(1);
      }
     // printf("mi %u  \n", response);
      if(response == -1) break;
      if (sendto(fd, &response, sizeof(response), 0, (SADDR *)&servaddr1,
                 sizeof(servaddr1)) == -1) {
        perror("sendto problem3");
        exit(1);
      }
    }

  close(sockfd);
  close(fd);
  exit(0);

}
