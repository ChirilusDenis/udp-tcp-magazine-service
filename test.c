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

int main() {
    char s1[] = "upd/precis/elevator/*/floor";
    char s2[] = "upd/precis/elevator/1/floor";
    if (mymatch(s1, s2)) {
        printf("OK\n");
    }
}