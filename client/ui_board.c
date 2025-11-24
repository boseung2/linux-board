// ui_board.c
#include "ui_board.h"

void ui_board_main(ClientContext *ctx) {
    char line[BUF_SIZE];

    while (ctx->running) {
        printf("\n=== 게시판 메인 (%s님) ===\n", ctx->user_id);
        printf("1. 게시글 목록 보기\n");
        printf("2. 게시글 작성\n");
        printf("3. 로그아웃\n");
        printf("선택: ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            ctx->running = 0;
            return;
        }

        int choice = atoi(line);
        if (choice == 1) {
            // TODO: LIST 명령
        } else if (choice == 2) {
            // TODO: WRITE 명령
        } else if (choice == 3) {
            printf("로그아웃합니다.\n");
            ctx->screen = SCREEN_MAIN_MENU;
            return;
        } else {
            printf("잘못된 선택입니다.\n");
        }
    }
}