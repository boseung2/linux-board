#include "ui_main_menu.h"
#include "socket.h"

// Helper to ensure client is connected
static int ensure_connection(ClientContext *ctx) {
    if (ctx->sock == -1) {
        printf("\n[안내] 서버와의 연결이 끊겼습니다. 재연결을 시도합니다...\n");
        if (net_connect(ctx) == -1) {
            printf("[오류] 서버에 연결할 수 없습니다. 잠시 후 다시 시도해 주세요.\n");
            sleep(2);
            return -1; // Failure
        }
        printf("[성공] 서버에 다시 연결되었습니다.\n");
        sleep(1);
    }
    return 0; // Success or already connected
}

void ui_main_menu_show() {
    system("clear"); 
    
    printf("=== 메인 메뉴 ===\n");
    printf("1. 로그인\n");
    printf("2. 회원가입\n");
    printf("3. 종료\n");
    printf("선택: ");
}

int ui_main_menu_handle_input(ClientContext *ctx) {
    char line[BUF_SIZE];
    if (fgets(line, sizeof(line), stdin) == NULL) {
        ctx->running = 0;
        return 0;
    }

    int choice = atoi(line);
    switch (choice) {
        case 1: // 로그인
            if (ensure_connection(ctx) != 0) {
                // 재연결 실패 시, 메인 메뉴 화면을 다시 표시하기 위해 루프를 계속
                return 0; 
            }
            ctx->screen = SCREEN_LOGIN;
            return 1;
        case 2: // 회원가입
            if (ensure_connection(ctx) != 0) {
                return 0;
            }
            ctx->screen = SCREEN_SIGNUP;
            return 1;
        case 3: { // 종료
            if (ctx->sock != -1) {
                const char *quit_msg = "QUIT\n";
                write(ctx->sock, quit_msg, strlen(quit_msg));
            }
            ctx->running = 0;
            return 1;
        }
        default:
            printf("잘못된 선택입니다.\n");
            sleep(1);
            return 0;
    }
}