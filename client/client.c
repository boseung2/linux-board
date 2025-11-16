// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 9000
#define BUF_SIZE    1024

int main() {
    int sock;
    struct sockaddr_in serv_addr;
    char buf[BUF_SIZE];

    // 1. 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    // 2. 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    // 3. 서버 연결
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        close(sock);
        exit(1);
    }

    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("Commands: SIGNUP <id> <pw>, LOGIN <id> <pw>, QUIT\n");

    fd_set reads, cpy_reads;
    int fd_max = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

    while (1) {
        FD_ZERO(&reads);
        FD_SET(STDIN_FILENO, &reads); // 키보드 입력 감시
        FD_SET(sock, &reads);         // 서버 소켓 감시

        int num_ready = select(fd_max + 1, &reads, NULL, NULL, NULL);
        if (num_ready == -1) {
            perror("select");
            break;
        }

        // 1) 키보드 입력이 들어온 경우
        if (FD_ISSET(STDIN_FILENO, &reads)) {
            if (fgets(buf, BUF_SIZE, stdin) == NULL) {
                // EOF (Ctrl+D)
                printf("stdin closed. exiting...\n");
                break;
            }

            // 입력을 서버로 전송
            if (write(sock, buf, strlen(buf)) == -1) {
                perror("write");
                break;
            }
        }

        // 2) 서버로부터 데이터가 온 경우
        if (FD_ISSET(sock, &reads)) {
            int len = read(sock, buf, BUF_SIZE - 1);
            if (len <= 0) {
                if (len == 0) {
                    printf("Server closed connection.\n");
                } else {
                    perror("read");
                }
                break;
            }
            buf[len] = '\0';
            printf("[SERVER] %s", buf);
        }
    }

    close(sock);
    return 0;
}