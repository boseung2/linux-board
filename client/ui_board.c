// client/ui_board.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "ui_board.h"

// 내부 헬퍼: 한 줄씩 읽기 (서버 → 클라이언트)
static int read_line(int sock, char *buf, size_t size) {
    size_t idx = 0;
    while (idx + 1 < size) {
        char c;
        ssize_t n = read(sock, &c, 1);
        if (n <= 0) {
            // 연결 끊김 또는 에러
            if (idx == 0) return -1;
            break;
        }
        buf[idx++] = c;
        if (c == '\n') break;
    }
    buf[idx] = '\0';
    return (int)idx;
}

// 내부 헬퍼: 한 줄 보내기 (클라이언트 → 서버)
static int send_line(int sock, const char *line) {
    size_t len = strlen(line);
    ssize_t n = write(sock, line, len);
    return (n == (ssize_t)len) ? 0 : -1;
}

// ===== 글 작성 화면 =====
static void board_screen_create(ClientContext *ctx) {
    char title[128];
    char content[2048];
    char line[256];

    memset(title, 0, sizeof(title));
    memset(content, 0, sizeof(content));

    system("clear");
    printf("====== 글 작성 ======\n");

    // 제목 입력
    printf("제목: ");
    if (fgets(title, sizeof(title), stdin) == NULL) {
        return;
    }
    title[strcspn(title, "\n")] = '\0';

    // 내용 입력 ('.' 단독 줄로 종료)
    printf("내용 (입력 후 .만 단독으로 입력하면 종료):\n\n");

    size_t len = 0;
    while (1) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        if (strcmp(line, ".\n") == 0 || strcmp(line, ".") == 0) {
            break;
        }
        size_t need = strlen(line);
        if (len + need >= sizeof(content) - 1) {
            printf("[경고] 내용이 너무 깁니다. 더 이상 입력할 수 없습니다.\n");
            break;
        }
        memcpy(content + len, line, need);
        len += need;
        content[len] = '\0';
    }

    printf("--------------------------------\n");
    printf("1. 등록\n");
    printf("2. 취소\n");
    printf("선택: ");

    if (fgets(line, sizeof(line), stdin) == NULL) return;
    int sel = atoi(line);
    if (sel == 2) {
        printf("[취소] 글 작성이 취소되었습니다.\n");
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    // 서버로 전송: POST 제목|내용
    char send_buf[BUF_SIZE];
    snprintf(send_buf, sizeof(send_buf), "POST %s|%s\n", title, content);
    if (send_line(ctx->sock, send_buf) != 0) {
        printf("[오류] 서버로 데이터를 전송하지 못했습니다.\n");
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    // 응답 읽기
    char recv_buf[BUF_SIZE];
    if (read_line(ctx->sock, recv_buf, sizeof(recv_buf)) <= 0) {
        printf("[오류] 서버 응답을 읽지 못했습니다.\n");
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    if (strncmp(recv_buf, "OK POST", 7) == 0) {
        int id = 0;
        sscanf(recv_buf, "OK POST %d", &id);
        printf("[완료] %d번 게시글이 등록되었습니다.\n", id);
    } else {
        printf("[실패] 글 작성에 실패했습니다.\n");
        printf("응답: %s", recv_buf);
    }

    printf("\n계속하려면 Enter 키를 누르세요...");
    fgets(line, sizeof(line), stdin);
}

// ===== 글 상세 화면 =====
static void board_screen_detail(ClientContext *ctx, int post_id) {
    char line[BUF_SIZE];

    system("clear");
    printf("====== 글 상세 ======\n");

    // 요청 전송
    char send_buf[64];
    snprintf(send_buf, sizeof(send_buf), "VIEW %d\n", post_id);
    if (send_line(ctx->sock, send_buf) != 0) {
        printf("[오류] 서버 전송 실패\n");
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    // 첫 줄: OK VIEW or FAIL ...
    if (read_line(ctx->sock, line, sizeof(line)) <= 0) {
        printf("[오류] 서버 응답 없음\n");
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    if (strncmp(line, "OK VIEW", 7) != 0) {
        printf("[실패] 글을 불러오지 못했습니다.\n");
        printf("응답: %s", line);
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    int id = 0, author_id = 0;
    char title[128] = {0};
    long epoch = 0;

    // ID 줄
    if (read_line(ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    sscanf(line, "ID %d", &id);

    // AUTHOR 줄
    if (read_line(ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    sscanf(line, "AUTHOR %d", &author_id);

    // TITLE 줄
    if (read_line(ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    // "TITLE " 이후 전체를 제목으로 사용
    if (strncmp(line, "TITLE ", 6) == 0) {
        strncpy(title, line + 6, sizeof(title) - 1);
        title[strcspn(title, "\n")] = '\0';
    }

    // DATE 줄
    if (read_line(ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    sscanf(line, "DATE %ld", &epoch);

    // CONTENT 헤더 줄
    if (read_line(ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    if (strncmp(line, "CONTENT", 7) != 0) {
        printf("[오류] 프로토콜 에러: CONTENT 헤더 없음\n");
        goto wait_enter;
    }

    // 내용 읽기 ('.' 한 줄 나올 때까지)
    char content[2048];
    content[0] = '\0';
    size_t len = 0;

    while (1) {
        if (read_line(ctx->sock, line, sizeof(line)) <= 0) break;
        if (strcmp(line, ".\n") == 0 || strcmp(line, ".") == 0) break;

        size_t need = strlen(line);
        if (len + need >= sizeof(content) - 1) {
            break;
        }
        memcpy(content + len, line, need);
        len += need;
        content[len] = '\0';
    }

    // 화면 출력
    printf("글번호: %d\n", id);
    printf("제목: %s\n", title);
    printf("작성자: %d\n", author_id); // TODO: author_id → 닉네임 매핑
    printf("작성일(UNIX epoch): %ld\n", epoch);
    printf("--------------------------------\n");
    printf("내용:\n%s", content);
    printf("--------------------------------\n");

    printf("1. 글 삭제 (임시)\n");
    printf("3. 목록으로 돌아가기\n");
    printf("선택: ");

    if (fgets(line, sizeof(line), stdin) == NULL) return;
    int sel = atoi(line);

    if (sel == 1) {
        // DELETE 요청
        char send_del[64];
        snprintf(send_del, sizeof(send_del), "DELETE %d\n", id);
        if (send_line(ctx->sock, send_del) != 0) {
            printf("[오류] 삭제 요청 전송 실패\n");
        } else {
            char resp[BUF_SIZE];
            if (read_line(ctx->sock, resp, sizeof(resp)) > 0) {
                if (strncmp(resp, "OK DELETE", 9) == 0) {
                    printf("[완료] 글이 삭제되었습니다.\n");
                } else {
                    printf("[실패] 글 삭제 실패: %s", resp);
                }
            }
        }
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
    }

    return;

read_error:
    printf("[오류] 서버로부터 데이터를 읽는 중 문제 발생\n");
wait_enter:
    printf("\n계속하려면 Enter 키를 누르세요...");
    fgets(line, sizeof(line), stdin);
}

// ===== 글 목록 화면 =====
static void board_screen_list(ClientContext *ctx) {
    char line[BUF_SIZE];

    while (1) {
        system("clear");
        printf("====== 글 목록 ======\n");

        // 서버에 목록 요청
        if (send_line(ctx->sock, "LIST\n") != 0) {
            printf("[오류] 서버 전송 실패\n");
            printf("\n계속하려면 Enter 키를 누르세요...");
            fgets(line, sizeof(line), stdin);
            return;
        }

        // 첫 줄: OK LIST count
        if (read_line(ctx->sock, line, sizeof(line)) <= 0) {
            printf("[오류] 서버 응답 없음\n");
            printf("\n계속하려면 Enter 키를 누르세요...");
            fgets(line, sizeof(line), stdin);
            return;
        }

        int count = 0;
        if (sscanf(line, "OK LIST %d", &count) != 1) {
            printf("[실패] 목록을 불러오지 못했습니다.\n");
            printf("응답: %s", line);
            printf("\n계속하려면 Enter 키를 누르세요...");
            fgets(line, sizeof(line), stdin);
            return;
        }

        // 목록 데이터 읽기
        struct {
            int id;
            char title[128];
            int author_id;
        } items[100];

        int actual = 0;
        for (int i = 0; i < count && i < 100; i++) {
            if (read_line(ctx->sock, line, sizeof(line)) <= 0) {
                break;
            }
            // 포맷: id|title|author_id
            char *p1 = strchr(line, '|');
            char *p2 = p1 ? strchr(p1 + 1, '|') : NULL;
            if (!p1 || !p2) continue;

            *p1 = '\0';
            *p2 = '\0';

            int id = atoi(line);
            char *title = p1 + 1;
            int author_id = atoi(p2 + 1);

            items[actual].id = id;
            strncpy(items[actual].title, title, sizeof(items[actual].title) - 1);
            items[actual].title[sizeof(items[actual].title) - 1] = '\0';
            items[actual].title[strcspn(items[actual].title, "\n")] = '\0';
            items[actual].author_id = author_id;

            actual++;
        }

        // 화면 출력
        printf("번호   제목                      작성자\n");
        for (int i = 0; i < actual; i++) {
            printf("%-5d %-24s %d\n",
                   items[i].id, items[i].title, items[i].author_id);
        }
        printf("-------------------------------------------\n");
        printf("선택:\n");
        printf("  [글번호] → 해당 글 상세보기\n");
        printf("  [C] 글 작성\n");
        printf("  [R] 새로고침\n");
        printf("  [B] 뒤로(게시판 메인으로)\n");
        printf("입력: ");

        if (fgets(line, sizeof(line), stdin) == NULL) return;
        // 개행 제거
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "B") == 0 || strcmp(line, "b") == 0) {
            return; // 게시판 메인으로
        } else if (strcmp(line, "R") == 0 || strcmp(line, "r") == 0) {
            continue; // 다시 목록
        } else if (strcmp(line, "C") == 0 || strcmp(line, "c") == 0) {
            board_screen_create(ctx);
        } else {
            // 숫자인지 확인 후 상세보기
            int id = atoi(line);
            if (id > 0) {
                board_screen_detail(ctx, id);
            } else {
                printf("[안내] 잘못된 입력입니다.\n");
                printf("\n계속하려면 Enter 키를 누르세요...");
                fgets(line, sizeof(line), stdin);
            }
        }
    }
}

// ===== 게시판 메인 화면 =====
void ui_board_main(ClientContext *ctx) {
    char line[64];
    int sel;

    while (ctx->running) {
        system("clear");
        printf("====== 게시판 메인 ======\n");
        printf("[로그인: %s]\n\n", ctx->user_id[0] ? ctx->user_id : "미로그인");

        printf("1. 글 목록 보기\n");
        printf("2. 글 작성\n");
        printf("3. 내 글 목록 (선택)\n");
        printf("4. 로그아웃\n");
        printf("5. 프로그램 종료\n\n");

        printf("선택: ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            ctx->running = 0;
            return;
        }
        sel = atoi(line);
        printf("\n");

        switch (sel) {
            case 1:
                board_screen_list(ctx);
                break;

            case 2:
                board_screen_create(ctx);
                break;

            case 3:
                printf("[안내] 내 글 목록 기능은 아직 구현되지 않았습니다.\n");
                printf("\n계속하려면 Enter 키를 누르세요...");
                fgets(line, sizeof(line), stdin);
                break;

            case 4:
                printf("로그아웃합니다.\n");
                ctx->user_id[0] = '\0';
                ctx->screen = SCREEN_MAIN_MENU;
                printf("\n계속하려면 Enter 키를 누르세요...");
                fgets(line, sizeof(line), stdin);
                return;

            case 5:
                printf("프로그램을 종료합니다.\n");
                ctx->running = 0;
                return;

            default:
                printf("잘못된 선택입니다. 다시 시도하세요.\n");
                printf("\n계속하려면 Enter 키를 누르세요...");
                fgets(line, sizeof(line), stdin);
                break;
        }
    }
}