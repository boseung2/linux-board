#include "ui_auth.h"

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
    if (write(ctx->sock, buf, strlen(buf)) == -1) {
        perror("write");
        return -1;
    }

    int len = read(ctx->sock, buf, sizeof(buf) - 1);
    if (len <= 0) {
        if (len == 0) printf("서버 연결이 종료되었습니다.\n");
        else perror("read");
        ctx->running = 0;
        return -1;
    }
    buf[len] = '\0';
    printf("[SERVER] %s", buf); 


    if (strstr(buf, "OK SIGNUP") != NULL) {
        printf("회원가입이 완료되었습니다. 메인 메뉴로 돌아갑니다.\n");
        ctx->screen = SCREEN_MAIN_MENU;
        return 0;
    } else {
        printf("회원가입에 실패했습니다. 다시 시도해 주세요.\n");
        ctx->screen = SCREEN_MAIN_MENU;   // 설계에 따라, or 다시 SIGNUP 유지
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
    if (write(ctx->sock, buf, strlen(buf)) == -1) {
        perror("write");
        return -1;
    }

    int len = read(ctx->sock, buf, sizeof(buf) - 1);
    if (len <= 0) {
        if (len == 0) printf("서버 연결이 종료되었습니다.\n");
        else perror("read");
        ctx->running = 0;
        return -1;
    }
    buf[len] = '\0';
    printf("[SERVER] %s", buf);

    if (strstr(buf, "OK LOGIN") != NULL) {
        printf("로그인 성공!\n");
        strncpy(ctx->user_id, id, sizeof(ctx->user_id));
        ctx->user_id[sizeof(ctx->user_id) - 1] = '\0';
        ctx->screen = SCREEN_BOARD;
        return 0;
    } else {
        printf("로그인 실패. ID 또는 PW를 확인해 주세요.\n");
        ctx->screen = SCREEN_MAIN_MENU;  // 실패 시 메인으로 또는 로그인 유지
        return 1;
    }
}