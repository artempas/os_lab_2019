#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>	
#include <sys/stat.h>
#include <stdbool.h>


//#define SERV_PORT 10050
//#define BUFSIZE 100
#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  const size_t kSize = sizeof(struct sockaddr_in);

  int lfd, cfd;
  int nread;

  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;
	  pid_t child_pid = 0;

  int pipeEnds[2];
pipe(pipeEnds);



  int SERV_PORT = -1;
  int BUFSIZE = -1;
  while (1) {
    int current_optind = optind ? optind : 1;
    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"bufsize", required_argument, 0, 0},
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
  if (SERV_PORT == -1 || BUFSIZE == -1) {
    fprintf(stderr, "Using: %s --port 20001 --bufsize 4\n", argv[0]);
    return 1;
  }

  char buf[BUFSIZE];
  printf("TCP-SERVER starts...\n");
  //Возвращает файловый дескриптор(>=0), который будет использоваться как ссылка на созданный коммуникационный узел
  //SOCK_STREAM для потоковых сокетов
  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket problem ");
    exit(1);
  }

  //параметры для настройки адреса сокета
  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);

  //настройка адреса сокета
  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind problem");
    exit(1);
  }

  //Cообщает уровню протокола, что сокет готов к принятию новых входящих соединений
  //Перевод сокета в пассивное (слушающее) состояние и создание очередей сокетов
  if (listen(lfd, 5) < 0) {
    perror("listen problem");
    exit(1);
  }

  //Слушаем в цикле
  while (1) {
    unsigned int clilen = kSize;

    //Является блокирующим – ожидает поступления запроса на соединение
    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept problem ");
      exit(1);
    }
    printf("connection established \n");

     int num = 0;
    if (read(cfd, &num, sizeof(int)) <= 0) {
      perror("read");
      exit(1);
    }

   

    int SERV_PORT = ntohs(cliaddr.sin_port);
    if (write(cfd, &SERV_PORT, sizeof(int)) < 0) {
      perror("write");
      exit(1);
    }
    child_pid = fork();
    if (child_pid == 0) {
      /////////////////////////// UDP code //////////////////
      int sockfd, n;
      char ipadr[16];
      struct sockaddr_in servaddr1;
      struct sockaddr_in cliaddr;
      bool checklist[num + 1];
      int all = 0;
      for (int i = 0; i < num; i++)
        checklist[i] = 0;


      if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket problem");
        exit(1);
      }
      
      memset(&servaddr, 0, sizeof(struct sockaddr_in));
      servaddr1.sin_family = AF_INET;
      servaddr1.sin_addr.s_addr = htonl(INADDR_ANY);
      servaddr1.sin_port = htons(SERV_PORT);


      if (bind(sockfd, (SADDR *)&servaddr1, sizeof(struct sockaddr_in)) < 0) {
        perror("bind problem");
        exit(1);
      }
      printf("UDP-SERVER starts...\n");
      int msg = 0;
      //sleep(1);

      while (true) {
        unsigned int len = sizeof(struct sockaddr_in);
        if ((n = recvfrom(sockfd, &msg, sizeof(msg), 0, (SADDR *)&cliaddr,
                          &len)) < 0) {
          perror("recvfrom");
          exit(1);
        }
        if (msg != 0) {
          checklist[msg] = 1;
          printf("REQUEST %u      FROM %s : %d\n", msg,
                 inet_ntop(AF_INET, &cliaddr.sin_addr.s_addr, ipadr, 16),
                 ntohs(cliaddr.sin_port));
        } else {
          checklist[0] = 1;
          int j;
          for (j = 1; j < num+1; j++) {
            if (checklist[j] != 1) {
              write(pipeEnds[1], &j, sizeof(int));
              checklist[0] = 0;
            }
          }
          if (checklist[0] == 1) {
            int k = -1;
            write(pipeEnds[1], &k, sizeof(int));
            break;
          } else {
            int k = 0;
            write(pipeEnds[1], &k, sizeof(int));
          }
        }
      }
      /////////////////////////////////////////////////
    }


    int missing = 0;
    while (true) {
      read(pipeEnds[0], &missing, sizeof(int));
       //printf("mi %u  \n", missing);
      if (write(cfd, &missing, sizeof(missing)) < 0) {
        perror("write problem");
        exit(1);
      }
     
    }
    if(missing == -1)
        kill(child_pid, SIGKILL);
  }

    //Закрывает (или прерывает) все существующие соединения сокета
    
  
    close(cfd);
    return 0;
}
