#include "ui_main_menu.h"

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
        case 1:
            ctx->screen = SCREEN_LOGIN;
            return 1;
        case 2:
            ctx->screen = SCREEN_SIGNUP;
            return 1;
        case 3: {
            const char *quit_msg = "QUIT\n";
            write(ctx->sock, quit_msg, strlen(quit_msg));
            ctx->running = 0;
            return 1;
        }
        default:
            printf("잘못된 선택입니다.\n");
            return 0;
    }
}