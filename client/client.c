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

typedef enum {
    SCREEN_MAIN_MENU,   // 메인 화면
    SCREEN_LOGIN,       // 로그인 화면
    SCREEN_SIGNUP,      // 회원가입 화면
    SCREEN_BOARD        // 게시판 메인 화면 (로그인 성공 후)
} ScreenState;

void show_main_menu() {
    printf("\n=== 메인 메뉴 ===\n");
    printf("1. 로그인\n");
    printf("2. 회원가입\n");
    printf("3. 종료\n");
    printf("선택: ");
}

int handle_main_menu_input(int sock) {
    char line[BUF_SIZE];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        return -1; // EOF
    }

    int choice = atoi(line);
    switch (choice) {
        case 1:
            return 1;  // 로그인
        case 2:
            return 2;  // 회원가입
        case 3:
            return 3;  // 종료
        default:
            printf("잘못된 선택입니다.\n");
            return 0;  // 다시
    }
}

int do_signup(int sock) {
    char id[32], pw[32];
    char buf[BUF_SIZE];

    printf("\n=== 회원가입 ===\n");
    printf("ID: ");
    if (fgets(id, sizeof(id), stdin) == NULL) return -1;
    id[strcspn(id, "\r\n")] = '\0';

    printf("PW: ");
    if (fgets(pw, sizeof(pw), stdin) == NULL) return -1;
    pw[strcspn(pw, "\r\n")] = '\0';

    // 서버로 전송
    snprintf(buf, sizeof(buf), "SIGNUP %s %s\n", id, pw);
    if (write(sock, buf, strlen(buf)) == -1) {
        perror("write");
        return -1;
    }

    // 서버 응답 읽기
    int len = read(sock, buf, sizeof(buf) - 1);
    if (len <= 0) {
        if (len == 0) printf("서버 연결이 종료되었습니다.\n");
        else perror("read");
        return -1;
    }
    buf[len] = '\0';
    printf("[SERVER] %s", buf);

    // 간단히 문자열 판별
    if (strncmp(buf, "OK SIGNUP", 9) == 0) {
        printf("회원가입이 완료되었습니다. 메인 메뉴로 돌아갑니다.\n");
        return 0; // 성공
    } else {
        printf("회원가입에 실패했습니다. 다시 시도해 주세요.\n");
        return 1; // 실패
    }
}

int do_login(int sock, char *out_user_id, size_t out_size) {
    char id[32], pw[32];
    char buf[BUF_SIZE];

    printf("\n=== 로그인 ===\n");
    printf("ID: ");
    if (fgets(id, sizeof(id), stdin) == NULL) return -1;
    id[strcspn(id, "\r\n")] = '\0';

    printf("PW: ");
    if (fgets(pw, sizeof(pw), stdin) == NULL) return -1;
    pw[strcspn(pw, "\r\n")] = '\0';

    // 서버로 전송
    snprintf(buf, sizeof(buf), "LOGIN %s %s\n", id, pw);
    if (write(sock, buf, strlen(buf)) == -1) {
        perror("write");
        return -1;
    }

    // 서버 응답
    int len = read(sock, buf, sizeof(buf) - 1);
    if (len <= 0) {
        if (len == 0) printf("서버 연결이 종료되었습니다.\n");
        else perror("read");
        return -1;
    }
    buf[len] = '\0';
    printf("[SERVER] %s", buf);

    if (strncmp(buf, "OK LOGIN", 8) == 0) {
        printf("로그인 성공!\n");
        strncpy(out_user_id, id, out_size);
        out_user_id[out_size - 1] = '\0';
        return 0; // 성공
    } else {
        printf("로그인 실패. ID 또는 PW를 확인해 주세요.\n");
        return 1; // 실패
    }
}

void board_main_loop(int sock, const char *user_id) {
    char line[BUF_SIZE];

    while (1) {
        printf("\n=== 게시판 메인 (%s님) ===\n", user_id);
        printf("1. 게시글 목록 보기\n");
        printf("2. 게시글 작성\n");
        printf("3. 로그아웃\n");
        printf("선택: ");

        if (fgets(line, sizeof(line), stdin) == NULL) return;

        int choice = atoi(line);
        if (choice == 1) {
            // TODO: 서버에 LIST 같은 명령 보내기
        } else if (choice == 2) {
            // TODO: WRITE 명령 구현
        } else if (choice == 3) {
            printf("로그아웃합니다.\n");
            return; // 메인 메뉴로 돌아가게 main에서 처리
        } else {
            printf("잘못된 선택입니다.\n");
        }
    }
}

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

    fd_set reads, cpy_reads;
    int fd_max = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

    ScreenState state = SCREEN_MAIN_MENU;
    int running = 1;
    char user_id[32] = {0};
    

    while (running) {
        if (state == SCREEN_MAIN_MENU) {
            show_main_menu();
            int choice = handle_main_menu_input(sock);

            if (choice == 1) {
                // 로그인
                int res = do_login(sock, user_id, sizeof(user_id));
                if (res == 0) {
                    state = SCREEN_BOARD;
                } else if (res < 0) {
                    // 에러 -> 종료
                    running = 0;
                }
            } else if (choice == 2) {
                // 회원가입
                int res = do_signup(sock);
                if (res < 0) {
                    running = 0;
                }
                // 성공/실패 상관없이 메인 메뉴로 돌아옴
            } else if (choice == 3) {
                // 종료
                const char *quit_msg = "QUIT\n";
                write(sock, quit_msg, strlen(quit_msg));
                running = 0;
            } else {
                // choice == 0 : 잘못된 입력
            }
        }
        else if (state == SCREEN_BOARD) {
            board_main_loop(sock, user_id);
            // 로그아웃 후 메인으로 복귀
            state = SCREEN_MAIN_MENU;
        }
    }


    close(sock);
    return 0;
}