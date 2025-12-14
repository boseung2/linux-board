#include "ui_auth.h"
#include "socket.h"

int ui_signup(ClientContext *ctx) {
    char id[32], pw[32];
    char buf[BUF_SIZE];

    printf("\n=== 회원가입 ===\n");
    printf("ID: ");
    if (fgets(id, sizeof(id), stdin) == NULL) return -1;
    id[strcspn(id, "\r\n")] = '\0';

    printf("PW: ");
    if (fgets(pw, sizeof(pw), stdin) == NULL) return -1;
    pw[strcspn(pw, "\r\n")] = '\0';

    snprintf(buf, sizeof(buf), "SIGNUP %s %s\n", id, pw);
    if (send_line(ctx->sock, buf) != 0) {
        perror("send_line");
        return -1;
    }

    if (read_line(ctx, ctx->sock, buf, sizeof(buf) - 1) <= 0) {
        if (ctx->sock != -1) { // 정상 종료가 아닌 경우
             printf("서버로부터 응답을 받지 못했습니다.\n");
        }
        // 자동 로그아웃 메시지는 read_line 내부에서 처리됨
        return -1;
    }

    printf("[SERVER] %s", buf); 

    if (strstr(buf, "OK SIGNUP") != NULL) {
        printf("회원가입이 완료되었습니다. 메인 메뉴로 돌아갑니다.\n");
        char dummy[8];
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(dummy, sizeof(dummy), stdin);
        ctx->screen = SCREEN_MAIN_MENU;
        return 0;
    } else {
        printf("회원가입에 실패했습니다. 다시 시도해 주세요.\n");
        char dummy[8];
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(dummy, sizeof(dummy), stdin);
        ctx->screen = SCREEN_MAIN_MENU;
        return 1;
    }
}

int ui_login(ClientContext *ctx) {
    char id[32], pw[32];
    char buf[BUF_SIZE];

    printf("\n=== 로그인 ===\n");
    printf("ID: ");
    if (fgets(id, sizeof(id), stdin) == NULL) return -1;
    id[strcspn(id, "\r\n")] = '\0';

    printf("PW: ");
    if (fgets(pw, sizeof(pw), stdin) == NULL) return -1;
    pw[strcspn(pw, "\r\n")] = '\0';

    snprintf(buf, sizeof(buf), "LOGIN %s %s\n", id, pw);
    if (send_line(ctx->sock, buf) != 0) {
        perror("send_line");
        return -1;
    }

    if (read_line(ctx, ctx->sock, buf, sizeof(buf) - 1) <= 0) {
        if (ctx->sock != -1) {
            printf("서버로부터 응답을 받지 못했습니다.\n");
        }
        return -1;
    }
    
    printf("[SERVER] %s", buf);

    if (strstr(buf, "OK LOGIN") != NULL) {
        printf("로그인 성공!\n");
        char dummy[8];
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(dummy, sizeof(dummy), stdin);
        strncpy(ctx->user_id, id, sizeof(ctx->user_id));
        ctx->user_id[sizeof(ctx->user_id) - 1] = '\0';
        ctx->screen = SCREEN_BOARD;
        return 0;
    } else {
        printf("로그인 실패. ID 또는 PW를 확인해 주세요.\n");
        char dummy[8];
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(dummy, sizeof(dummy), stdin);
        ctx->screen = SCREEN_MAIN_MENU;
        return 1;
    }
}