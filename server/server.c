#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/select.h>

#include "user.h"
#include "auth.h"
#include "board_service.h"
#include "board_controller.h"
#include "log.h"

#define PORT 9000
#define MAX_CLIENTS FD_SETSIZE
#define BUF_SIZE 1024

static void handle_command(int fd, char *line) {
    char cmd[16], arg1[64], arg2[64];

    // 개행 제거
    line[strcspn(line, "\r\n")] = '\0';
    LOG_DEBUG("raw command from fd=%d: '%s'", fd, line);

    int n = sscanf(line, "%15s %63s %63s", cmd, arg1, arg2);
    if (n <= 0) {
        LOG_WARN("Empty command (fd=%d)", fd);
        const char *msg = "FAIL EMPTY_COMMAND\n";
        write(fd, msg, strlen(msg));
        return;
    }

    // 회원가입
    if (strcmp(cmd, "SIGNUP") == 0) {
        if (n < 3) {
            const char *msg = "FAIL SIGNUP INVALID_ARGS\n";
            LOG_WARN("SIGNUP invalid args (fd=%d, line='%s')", fd, line);
            write(fd, msg, strlen(msg));
            return;
        }
        handle_signup(fd, arg1, arg2);
    }

    // 로그인
    else if (strcmp(cmd, "LOGIN") == 0) {
        if (n < 3) {
            const char *msg = "FAIL LOGIN INVALID_ARGS\n";
            LOG_WARN("LOGIN invalid args (fd=%d, line='%s')", fd, line);
            write(fd, msg, strlen(msg));
            return;
        }
        handle_login(fd, arg1, arg2);
    }

    // 종료
    else if (strcmp(cmd, "QUIT") == 0) {
        handle_quit(fd);
        // 여기서 close(fd)를 할지, 메인 루프에서 할지는 정책에 따라
    }

    // 게시판 관련 명령 (예: POST, LIST 등) 가정
    else if (strcmp(cmd, "POST") == 0 ||
         strcmp(cmd, "LIST") == 0 ||
         strcmp(cmd, "VIEW") == 0 ||
         strcmp(cmd, "DELETE") == 0 ||
         strcmp(cmd, "UPDATE") == 0) {
    // line = "POST ..." 전체 문자열이라고 가정
    // cmd 길이를 알고 있다면:
    size_t cmdlen = strlen(cmd);
    const char *args = (n >= 2) ? line + cmdlen + 1 : "";
    handle_board_command(fd, cmd, args);
    }

    else {
        const char *msg = "FAIL UNKNOWN_COMMAND\n";
        LOG_WARN("Unknown command (fd=%d, cmd=%s)", fd, cmd);
        write(fd, msg, strlen(msg));
    }
}

int main() {
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_len;

    fd_set reads, cpy_reads;
    int fd_max;

    char buf[BUF_SIZE];

    user_system_init();
    LOG_INFO("User system initialized");

    board_system_init();
    LOG_INFO("Board system initialized");

    // 소켓 생성
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        LOG_ERROR("socket() error");
        perror("socket() error");
        exit(1);
    }

    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        LOG_ERROR("bind() error");
        perror("bind() error");
        close(serv_sock);
        exit(1);
    }

    if (listen(serv_sock, 5) == -1) {
        LOG_ERROR("listen() error");
        perror("listen() error");
        close(serv_sock);
        exit(1);
    }

    LOG_INFO("Server is running on port %d", PORT);
    printf("Server is running on port %d\n", PORT);

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    while (1) {
        cpy_reads = reads;
        int num_ready = select(fd_max + 1, &cpy_reads, NULL, NULL, NULL);
        if (num_ready == -1) {
            LOG_ERROR("select() error");
            perror("select() error");
            break;
        }

        for (int fd = 0; fd <= fd_max; fd++) {
            if (!FD_ISSET(fd, &cpy_reads)) continue;

            if (fd == serv_sock) {
                clnt_addr_len = sizeof(clnt_addr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_len);
                if (clnt_sock == -1) {
                    LOG_ERROR("accept() error");
                    perror("accept");
                    continue;
                }

                FD_SET(clnt_sock, &reads);
                if (clnt_sock > fd_max) fd_max = clnt_sock;

                LOG_INFO("New client connected: fd=%d", clnt_sock);
                // const char *msg = "Welcome! Use SIGNUP/LOGIN/QUIT\n";
                // write(clnt_sock, msg, strlen(msg));
            }
            else {
                int len = read(fd, buf, BUF_SIZE - 1);
                if (len <= 0) {
                    if (len == 0) {
                        LOG_INFO("Client disconnected: fd=%d", fd);
                    } else {
                        LOG_ERROR("read() error on fd=%d", fd);
                        perror("read");
                    }
                    close(fd);
                    FD_CLR(fd, &reads);
                } else {
                    buf[len] = '\0';
                    handle_command(fd, buf);
                }
            }
        }
    }

    close(serv_sock);
    LOG_INFO("Server shutdown");
    return 0;
}