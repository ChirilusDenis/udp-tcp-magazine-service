#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct tcp_msg {
    int tot_len;
    struct in_addr ip;
    uint16_t src_port;
    char topic[51];
    uint8_t type;
    char data[1501];
};

int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  size_t crt;
  
    while(bytes_remaining) {
      crt = recv(sockfd, buff + bytes_received, len, 0);
      if (crt <= 0) break;
      bytes_remaining -= crt;
      bytes_received += crt;
    }
  return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  size_t crt;

    while(bytes_remaining) {
      crt = send(sockfd, buff + bytes_sent, len, 0);
      bytes_remaining -= crt;
      bytes_sent += crt;
    }
  return bytes_sent;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
      fprintf(stderr, "Usage subscriber <ID> <SERVER IP> <SERVER PORT>\n");
      return 1;
    }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    char *id = argv[1];
    char *servip = argv[2];
    uint16_t port;
    int yes = 1;
    char BUFF[100];
    struct tcp_msg in_msg;

    int rc = sscanf(argv[3], "%hu", &port);
    if(rc != 1) {
      fprintf(stderr, "sscanf\n");
      return 1;
      }

    struct sockaddr_in serv;
    socklen_t len = sizeof(struct sockaddr);
    memset(&serv, 0, len);
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    rc = inet_pton(AF_INET, servip, &serv.sin_addr.s_addr);
    if (rc <= 0) {
      fprintf(stderr, "inet_pton\n");
      return 1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      fprintf(stderr, "socket\n");
      return 1;
      }
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(int));
    rc = connect(sockfd, (struct sockaddr *)&serv, len);
    if (rc < 0) {
      fprintf(stderr, "connect\n");
      return 1;
      }

    rc = send_all(sockfd, id, 10);
    if (rc <= 0) {
      fprintf(stderr, "send\n");
      return 1;
    }

    struct pollfd pols[3];
    pols[0].fd = 0;
    pols[0].events = POLLIN;

    pols[1].fd = sockfd;
    pols[1].events = POLLIN;

    while (1) {
        rc = poll(pols, 2, -1);
        if (rc < 0) {
          fprintf(stderr, "poll\n");
          return 1;
          }

        for(int i = 0; i < 2; i++)  {
            if(pols[i].revents & POLLIN) {
                if (i == 0) {
                    fgets(BUFF, 99, stdin);
                    for (int i = 0; i < 99; i++) {
                      if (BUFF[i] == '\n') {
                        BUFF[i] = '\0';
                        break;
                      }
                    }
                    if(strncmp(BUFF, "subscribe", 9) == 0) {
                        BUFF[9] = '+';
                        send_all(sockfd, BUFF + 9, 51);
                        if (rc <= 0) {
                          fprintf(stderr, "send\n");
                          return 1;
                        }
                        printf("Subscribed to topic %s\n", BUFF + 10);
                    } else if (strncmp(BUFF, "unsubscribe", 11) == 0) {
                        BUFF[11] ='-';
                        send_all(sockfd, BUFF + 11, 51);
                        if (rc <= 0) {
                          fprintf(stderr, "send\n");
                          return 1;
                        }
                        printf("Unsubscribed from topic %s\n", BUFF + 12);
                    } else if(strncmp(BUFF, "exit", 4) == 0) {
                        close(sockfd);
                        return 0;    
                    }

                } else {
                    rc = recv_all(sockfd, &in_msg, 4);
                    rc = recv_all(sockfd, (char *)(&in_msg) + 4, in_msg.tot_len - 4);
                    if (rc <= 0) return 0;
                    printf("%s:%hu - %s - ", inet_ntoa(in_msg.ip), ntohs(in_msg.src_port), in_msg.topic);
                    switch (in_msg.type) {
                      case 0 :
                        if (in_msg.data[0] == 0) {
                          printf("INT - %u\n", ntohl(*((uint32_t *)(in_msg.data + 1))));
                        } else {
                          printf("INT - %d\n", - 1 * ntohl(*((uint32_t *)(in_msg.data + 1))));
                        }
                        break;

                      case 1:
                        printf("SHORT_REAL - %.2f\n", ntohs(*((uint16_t *)in_msg.data)) / 100.0);
                        break;

                      case 2:
                        uint32_t base = ntohl(*((uint32_t *)(in_msg.data + 1)));
                        uint8_t exp = (uint8_t)in_msg.data[5];
                        double number = 0.0 + base;
                        for (uint8_t i = 0; i< exp; i++) {number /= 10;}
                        if (in_msg.data[0] == 0) {
                          printf("FLOAT - %f\n", number);
                        } else {
                          printf("FLOAT - %f\n", -1.0 * number);
                        }
                        break;

                      default:
                        printf("STRING - %s\n", in_msg.data);
                        break;
                    }
                }
            }
        }
    }


    return 0;
}