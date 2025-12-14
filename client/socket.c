#include "socket.h"

int net_connect(ClientContext *ctx) {
    int sock;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        close(sock);
        return -1;
    }

    ctx->sock = sock;
    return 0;
}

// 한 줄씩 읽기 (서버 → 클라이언트)
// 자동 로그아웃 처리를 포함한다.
int read_line(ClientContext *ctx, int sock, char *buf, size_t size) {
    size_t idx = 0;
    while (idx + 1 < size) {
        char c;
        // 소켓이 유효한지 확인
        if (sock < 0) {
            if (idx == 0) return -1;
            break;
        }
        ssize_t n = read(sock, &c, 1);
        if (n <= 0) {
            if (ctx->sock != -1) {
                printf("\n[오류] 서버와의 연결이 끊겼습니다.\n");
                 // 스트림을 비워, 자동 로그아웃 메시지 후의 불필요한 입력을 방지
                fflush(stdout);
            }
            if (idx == 0) return -1;
            break;
        }
        buf[idx++] = c;
        if (c == '\n') break;
    }
    buf[idx] = '\0';

    if (strcmp(buf, "LOGOUT_INACTIVE\n") == 0) {
        system("clear");
        printf("\n[안내] 5분 동안 활동이 없어 자동으로 로그아웃되었습니다.\n");
        printf("[안내] 잠시 후 메인 메뉴로 돌아갑니다.\n");
        
        ctx->user_id[0] = '\0';
        ctx->screen = SCREEN_MAIN_MENU;
        
        if(ctx->sock != -1) {
            close(ctx->sock);
            ctx->sock = -1;
        }
        
        sleep(2); // 메시지를 사용자가 읽을 수 있도록 잠시 대기
        
        return -1; // 연결 끊김으로 처리
    }

    return (int)idx;
}

// 한 줄 보내기 (클라이언트 → 서버)
int send_line(int sock, const char *line) {
    size_t len = strlen(line);
    ssize_t n = write(sock, line, len);
    return (n == (ssize_t)len) ? 0 : -1;
}