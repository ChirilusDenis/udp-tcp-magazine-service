#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct client {
    int fd;
    char id[11];
    char *subs;
    int max_subs_size;
    int crt_sub;
    int num_subs;
};

struct upd_msg {
    char topic[50];
    uint8_t type;
    char data[1500];
};

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

bool mymatch(char *s1, char *s2) {
    for (int i = 0, j = 0; i < strlen(s1) && j < strlen(s2); i++, j++) {
    if (s1[i] == '\0' && s2[j] == '\0') return true;
    if (s1[i] == '\0' || s2[j] == '\0') return false;

    if (s1[i] == '+') {
        for (j; s2[j] != '\0'; j++) {
            if (s2[j] == '/') break;
        }
        i++;
    }

    if (s1[i] == '*') {
        if (s1[i + 1] == '\0') return true;
        i+=2;
        int len = 0;
        for (len; s1[i + len] != '\0' && s1[i + len] != '/'; len++);
        for (j; j < strlen(s2);j++) {
            if (s2[j] == '/' && strncmp(s1 + i, s2 + j + 1, len) == 0) {
                j++;
                break;
            }
            if (s2[j] == '\0') return false;
        }
    }

    if (s1[i] != s2[j]) return false;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "server <SERVER PORT>\n");
        return 1;
        }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int yes = 1;
    uint16_t port;
    int sock_in_fd;
    char BUFF[1000];
    struct upd_msg udp_in;
    struct sockaddr_in upd;
    socklen_t len = sizeof(struct sockaddr_in);
    int num_max_fd = 100;
    struct pollfd *pols = malloc(num_max_fd * sizeof(struct pollfd));
    if (pols == NULL) {
        fprintf(stderr, "malloc");
        return 1;
    }
    int num_fd = 3;
    pols[0].fd = 0;
    pols[0].events = POLLIN;

    int rc = sscanf(argv[1], "%hu", &port);
    if(rc != 1) {
        fprintf(stderr, "sscanf\n");
        return 1;
        }

    sock_in_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_in_fd < 0) {
        fprintf(stderr, "socket\n");
        return 1;
        }
    pols[1].fd = sock_in_fd;
    pols[1].events = POLLIN;

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        fprintf(stderr, "socket\n");
        return 1;
        }

    setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(int));
    struct sockaddr_in serv;
    memset(&serv, 0, len);
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    rc = inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr.s_addr);
    if (rc <= 0) {
        fprintf(stderr, "inet_pton\n");
        return 1;}

    rc = bind(listenfd, (struct sockaddr *)&serv, len);
    if (rc < 0) {
        fprintf(stderr, "bind\n");
        return 1;
        }
    rc = bind(sock_in_fd, (struct sockaddr *)&serv, len);
    if (rc < 0) {
        fprintf(stderr, "bind\n");
        return 1;
        }
    pols[2].fd = listenfd;
    pols[2].events = POLLIN;

    rc = listen(listenfd, 5);
    if (rc < 0) {
        fprintf(stderr, "listen\n");
        return 1;
        }

    int num_max_clients = 100;s
    struct client *clients = malloc(num_max_clients * sizeof(struct client));
    if (clients == NULL) {
        fprintf(stderr, "malloc");
        return 1;
    }
    int num_client = 0;

    while (1) {
        rc = poll(pols, num_fd, -1);
        if (rc < 0) {
            fprintf(stderr, "poll\n");
            return 1;
            }

        for(int i = 0; i < num_fd; i++)  {
            if (pols[i].revents & POLLIN) {
                if (pols[i].fd == 0) {
                    scanf("%s", BUFF);
                    if (strncmp(BUFF, "exit", 4) == 0) {
                        for(int i = 0; i < num_fd; i++) {
                            close(pols[i].fd);
                        }
                        free(pols);
                        for (int i = 0; i < num_client; i++) {
                            free(clients[i].subs);
                        }
                        free(clients);
                        return 0;
                    }
                    break;

                } else if (pols[i].fd == sock_in_fd) {
                    rc = recvfrom(sock_in_fd, &udp_in, 1556, 0, (struct sockaddr *)&upd, &len);
                    if (rc < 0) {
                        fprintf(stderr, "recvfrom\n");
                        return 1;
                    }

                    struct tcp_msg to_send;
                    to_send.ip = upd.sin_addr;
                    to_send.src_port = upd.sin_port;
                    to_send.type = udp_in.type;
                    memcpy(to_send.topic, udp_in.topic, 50);
                    to_send.topic[50] = '\0';
                    memcpy(to_send.data, udp_in.data, rc - 51);
                    if (to_send.type == 3) {
                        to_send.data[rc - 51] = '\0';
                    }
                    to_send.tot_len = 4 + sizeof(struct in_addr) + 2 + rc + 2;

                    for (int j = 0; j < num_client; j++) {
                        struct client client = clients[j];
                        int index = 0;
                        for (int k = 0; k < client.num_subs; k++) {
                            if (client.fd != -1 && mymatch(client.subs + index, to_send.topic)) {
                                rc = send_all(client.fd, &to_send, to_send.tot_len);
                                if (rc < 0) {
                                    fprintf(stderr, "send\n");
                                    return 1;
                                }
                                break;
                            }
                            index += strlen(client.subs + index) + 1;
                        }
                    }
                    break;

                } else if (pols[i].fd == listenfd) {
                    struct sockaddr_in tmp;
                    int tmpfd = accept(listenfd, (struct sockaddr *)&tmp, &len);
                    if (tmpfd < 0) {
                        fprintf(stderr, "accept\n");
                        return 1;
                        }
                    setsockopt(tmpfd, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof(int));
                    char idclient[11];
                    rc = recv_all(tmpfd, idclient, 10);
                    if (rc <= 0) {
                        fprintf(stderr, "recv\n");
                        return 1;
                        }
                    idclient[rc] = '\0';

                    int ok = 0;
                    for (int j = 0; j < num_client; j++) {
                        if(strcmp(clients[j].id, idclient) == 0) {
                            if (clients[j].fd != -1) {
                                printf("Client %s already connected.\n", idclient);
                                close(tmpfd);
                            } else {
                                clients[j].fd = tmpfd;
                                pols[num_fd].fd = tmpfd;
                                pols[num_fd].events = POLLIN;
                                num_fd++;
                                printf("New client %s connected from %s:%hu.\n", idclient, inet_ntoa(tmp.sin_addr), port);
                            }
                            ok = 1;
                            break;
                        }
                    }
                    if(ok) break;

                    if (num_client == num_max_clients - 1) {
                        num_max_clients *= 2;
                        struct client *tmpclt = realloc(clients, num_max_clients * sizeof(struct client));
                        if (tmpclt == NULL) {
                            fprintf(stderr, "realloc");
                            return 1;
                        }
                    }

                    clients[num_client].fd = tmpfd;
                    clients[num_client].crt_sub = 0;
                    clients[num_client].num_subs = 0;
                    strcpy(clients[num_client].id, idclient);
                    pols[num_fd].fd = clients[num_client].fd;
                    pols[num_fd].events = POLLIN;
                    clients[num_client].max_subs_size = 500;
                    clients[num_client].subs = malloc(clients[num_client].max_subs_size);
                    if (clients[num_client].subs == NULL) {
                        fprintf(stderr, "malloc");
                        return 1;
                    }
                    printf("New client %s connected from %s:%hu.\n", idclient, inet_ntoa(tmp.sin_addr), port);
                    num_client++;
                    num_fd++;
                    break;

                } else if (i > 2) {
                    for (int j = 0; j < num_client; j++) {
                        if (clients[j].fd == pols[i].fd) {
                            rc = recv_all(clients[j].fd, BUFF, 51);
                            if (rc <= 0) {
                                printf("Client %s disconnected.\n", clients[j].id);
                                close(pols[i].fd);
                                
                                clients[j].fd = -1;

                                for (int k = i; k < num_fd - 1; k++) {
                                    memcpy(pols + k, pols + k + 1, sizeof(struct pollfd));
                                }
                                num_fd--;

                                break;
                            }
                            if (BUFF[0] == '+') {
                                if (clients[j].crt_sub >= clients[j].max_subs_size - 50) {
                                    clients[j].max_subs_size *= 2;
                                    char *tmpsubs = realloc(clients[j].subs, clients[j].max_subs_size);
                                    if (tmpsubs == NULL) {
                                        fprintf(stderr, "realloc");
                                        return 1;
                                    }
                                    clients[j].subs = tmpsubs;
                                }
                                strcpy(clients[j].subs + clients[j].crt_sub, BUFF + 1);
                                clients[j].crt_sub += strlen(BUFF + 1) + 1;
                                clients[j].num_subs ++;
                            } else if (BUFF[0] == '-') {
                                int indexrm = 0;
                                 for (int k = 0; k < clients[j].num_subs; k++) {
                                    if (strcmp(BUFF + 1, clients[j].subs + indexrm) == 0) {
                                        int todel = strlen(clients[j].subs + indexrm);
                                        int total = clients[j].crt_sub + strlen(clients[j].subs + clients[j].crt_sub) + 1;
                                        memcpy(clients[j].subs + indexrm, clients[j].subs + indexrm + todel + 1, total - indexrm - todel);
                                        clients[j].num_subs --;
                                        clients[j].crt_sub -= todel + 1;
                                        break;
                                    }
                                    indexrm+=strlen(clients[j].subs) + 1;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}