// client/ui_board.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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
        line[strcspn(line, "\r\n")] = '\0';
        const char *br = "<BR>";
        size_t need = strlen(line) + strlen(br);
        if (len + need >= sizeof(content) - 1) {
            printf("[경고] 내용이 너무 깁니다. 더 이상 입력할 수 없습니다.\n");
            break;
        }
        memcpy(content + len, line, strlen(line));
        len += strlen(line);
        memcpy(content + len, br, strlen(br));
        len += strlen(br);
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

    // 서버로 전송: POST author_id 제목|내용
    char send_buf[BUF_SIZE];
    snprintf(send_buf, sizeof(send_buf), "POST %s|%s|%s\n", ctx->user_id, title, content);
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

// ===== 글 수정 화면 =====
static void board_screen_update(ClientContext *ctx, 
                                int post_id,
                                const char *old_title,
                                const char *old_content,
                                char *new_title,
                                char *new_content) {

    char line[BUF_SIZE];
    char input_title[128];
    char input_content[2048];

    system("clear");
    printf("====== 글 수정 ======\n");

    // 제목 입력
    printf("기존 제목: %s\n", old_title);
    printf("새 제목(Enter 시 기존 제목 유지): ");
    if (fgets(input_title, sizeof(input_title), stdin) == NULL) {
        return;
    }
    input_title[strcspn(input_title, "\n")] = '\0';

    // Enter 시 기존 제목 유지
    if (strlen(input_title) == 0) {
        strncpy(input_title, old_title, sizeof(input_title) - 1);
        input_title[sizeof(input_title) - 1] = '\0';
    }

    // 내용 입력 ('.' 단독 줄로 종료)
    printf("기존 내용: \n");
    for (size_t i = 0; old_content[i] != '\0'; ) {
        if (strncmp(&old_content[i], "<BR>", 4) == 0) {
            printf("\n");
            i += 4;
        } else {
            putchar(old_content[i]);
            i++;
        }
    }

    printf("새 내용 (입력 후 .만 단독으로 입력하면 종료):\n\n");

    input_content[0] = '\0';
    size_t len = 0;
    while (1) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        if (strcmp(line, ".\n") == 0 || strcmp(line, ".") == 0) {
            break;
        }
        line[strcspn(line, "\r\n")] = '\0';
        const char *br = "<BR>";
        size_t need = strlen(line) + strlen(br);
        if (len + need >= sizeof(input_content) - 1) {
            printf("[경고] 내용이 너무 깁니다. 더 이상 입력할 수 없습니다.\n");
            break;
        }
        memcpy(input_content + len, line, strlen(line));
        len += strlen(line);
        memcpy(input_content + len, br, strlen(br));
        len += strlen(br);
        input_content[len] = '\0';
    }
    strcpy(new_title, input_title);
    strcpy(new_content, input_content);
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
        printf("[안내] 이미 삭제된 글입니다.\n");
        printf("응답: %s", line);
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    int id = 0;
    char author_id[32] = {0};
    char title[128] = {0};
    long epoch = 0;

    // ID 줄
    if (read_line(ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    sscanf(line, "ID %d", &id);

    // AUTHOR 줄
    if (read_line(ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    sscanf(line, "AUTHOR %s", author_id);
    author_id[strcspn(author_id, "\n")] = '\0';

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
        if (strcmp(line, ".\n") == 0 || strcmp(line, ".") == 0 || strcmp(line, ".\r\n") == 0) break;

        size_t need = strlen(line);
        if (len + need >= sizeof(content) - 1) {
            break;
        }
        memcpy(content + len, line, need);
        len += need;
        content[len] = '\0';
    }

    // 시간 변환 (epoch -> YYYY-MM-DD HH:MM:SS)
    char time_buf[20];
    time_t raw_time = (time_t)epoch;
    struct tm *time_info = localtime(&raw_time);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", time_info);

    // 화면 출력
    printf("글번호: %d\n", id);
    printf("제목: %s\n", title);
    printf("작성자: %s\n", author_id);
    printf("작성일: %s\n", time_buf);
    printf("--------------------------------\n");
    printf("내용:\n");

    for (size_t i = 0; content[i] != '\0'; ) {
        if (strncmp(&content[i], "<BR>", 4) == 0) {
            printf("\n");
            i += 4;
        } else {
            putchar(content[i]);
            i++;
        }
    }

    printf("--------------------------------\n");

    printf("1. 글 수정\n");
    printf("2. 글 삭제\n");
    printf("3. 목록으로 돌아가기\n");
    printf("선택: ");

    if (fgets(line, sizeof(line), stdin) == NULL) return;
    int sel = atoi(line);

    if (sel == 1) {
        // 수정 권한 요청
        char perm_req[64]; 
        snprintf(perm_req, sizeof(perm_req), "CHKPRM %d|UPDATE\n", id);

        if (send_line(ctx->sock, perm_req) != 0) {
            printf("\n[오류] 권한 요청 전송 실패\n");
            goto wait_enter;
        }

        // 응답 대기
        char perm_resp[BUF_SIZE];
        if (read_line(ctx->sock, perm_resp, sizeof(perm_resp)) <= 0) {
            printf("\n[오류] 권한 응답 없음\n");
            goto wait_enter;
        }

        if (strncmp(perm_resp, "OK CHKPRM GRANTED", strlen("OK CHKPRM GRANTED")) != 0) {
            printf("\n[실패] 수정 권한이 없습니다.\n");
            printf("응답: %s", perm_resp);
            goto wait_enter;
        }

        // UPDATE 요청
        char new_title[128];
        char new_content[2048];
        board_screen_update(ctx, id, title, content, new_title, new_content);

        // 서버로 전송: UPDATE id 제목|내용
        char send_upd[BUF_SIZE];
        snprintf(send_upd, sizeof(send_upd), "UPDATE %d|%s|%s\n", id, new_title, new_content);
        if (send_line(ctx->sock, send_upd) != 0) {
            printf("[오류] 수정 요청 전송 실패\n");
        } else {
            char resp[BUF_SIZE];
            if (read_line(ctx->sock, resp, sizeof(resp)) > 0) {
                if (strncmp(resp, "OK UPDATE", 9) == 0) {
                    printf("[완료] 글이 수정되었습니다.\n");
                } else {
                    printf("[실패] 글 수정 실패: %s", resp);
                }
            }
        }
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
    }

    else if (sel == 2) {
        // 삭제 권한 요청
        char perm_req[64];
        snprintf(perm_req, sizeof(perm_req), "CHKPRM %d|DELETE\n", id);

        if (send_line(ctx->sock, perm_req) != 0) {
            printf("\n[오류] 권한 요청 전송 실패\n");
            goto wait_enter;
        }

        // 응답 대기
        char perm_resp[BUF_SIZE];
        if (read_line(ctx->sock, perm_resp, sizeof(perm_resp)) <= 0) {
            printf("\n[오류] 권한 응답 없음\n");
            goto wait_enter;
        }

        if (strncmp(perm_resp, "OK CHKPRM GRANTED", strlen("OK CHKPRM GRANTED")) != 0) {
            printf("\n[실패] 삭제 권한이 없습니다.\n");
            printf("응답: %s", perm_resp);
            goto wait_enter;
        }

        // DELETE 확인
        char YN[8];
        printf("정말 이 글을 삭제하시겠습니까? (Y/N): ");
        if (fgets(YN, sizeof(YN), stdin) == NULL) goto wait_enter;
        if (YN[0] != 'Y' && YN[0] != 'y') {
            printf("[취소] 글 삭제가 취소되었습니다.\n");
            goto wait_enter;
        }

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
static void board_screen_list(ClientContext *ctx, const char *initial_author_id) {
    char line[BUF_SIZE];
    int page = 0;
    const int limit = 10;
    char search_type[32] = {0};
    char keyword[128] = {0};

    // If initial_author_id is provided, set search criteria to list user's own posts
    if (initial_author_id != NULL && initial_author_id[0] != '\0') {
        strcpy(search_type, "author");
        strncpy(keyword, initial_author_id, sizeof(keyword) - 1);
        keyword[sizeof(keyword) - 1] = '\0';
    }

    while (1) {
        system("clear");
        printf("====== 글 목록 (페이지: %d) ======\n", page + 1);
        if (search_type[0] != '\0') {
            printf(">> 검색 모드 (%s: '%s') <<\n\n", 
                   strcmp(search_type, "title") == 0 ? "제목" : "작성자", 
                   keyword);
        }

        // 서버에 목록 요청 (offset, limit, search 사용)
        char send_buf[256];
        int offset = page * limit;
        if (search_type[0] != '\0') {
            snprintf(send_buf, sizeof(send_buf), "LIST %d %d %s %s\n", offset, limit, search_type, keyword);
        } else {
            snprintf(send_buf, sizeof(send_buf), "LIST %d %d\n", offset, limit);
        }
        
        if (send_line(ctx->sock, send_buf) != 0) {
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
            char author_id[32];
            long created_at;
            long updated_at;
        } items[100];

        int actual = 0;
        for (int i = 0; i < count && i < 100; i++) {
            if (read_line(ctx->sock, line, sizeof(line)) <= 0) {
                break;
            }
            // 포맷: id|title|author_id|created_at_epoch|updated_at_epoch
            char *p1 = strchr(line, '|');
            char *p2 = p1 ? strchr(p1 + 1, '|') : NULL;
            char *p3 = p2 ? strchr(p2 + 1, '|') : NULL;
            char *p4 = p3 ? strchr(p3 + 1, '|') : NULL;
            if (!p1 || !p2 || !p3 || !p4) continue;

            *p1 = '\0';
            *p2 = '\0';
            *p3 = '\0';
            *p4 = '\0';

            int id = atoi(line);
            char *title = p1 + 1;
            char *author_id_str = p2 + 1;
            long created_at_epoch = atol(p3 + 1);
            long updated_at_epoch = atol(p4 + 1);

            items[actual].id = id;
            strncpy(items[actual].title, title, sizeof(items[actual].title) - 1);
            items[actual].title[sizeof(items[actual].title) - 1] = '\0';
            strncpy(items[actual].author_id, author_id_str, sizeof(items[actual].author_id) - 1);
            items[actual].author_id[sizeof(items[actual].author_id) - 1] = '\0';
            items[actual].created_at = created_at_epoch;
            items[actual].updated_at = updated_at_epoch;

            actual++;
        }

        // 화면 출력
        printf("번호   제목                      작성자   작성일              수정일\n");
        for (int i = 0; i < actual; i++) {
            char created_time_buf[20]; // "YYYY-MM-DD HH:MM:SS\0"
            char updated_time_buf[20]; // "YYYY-MM-DD HH:MM:SS\0"

            time_t raw_created_time = (time_t)items[i].created_at;
            struct tm *created_time_info = localtime(&raw_created_time);
            strftime(created_time_buf, sizeof(created_time_buf), "%Y-%m-%d %H:%M:%S", created_time_info);

            time_t raw_updated_time = (time_t)items[i].updated_at;
            struct tm *updated_time_info = localtime(&raw_updated_time);
            strftime(updated_time_buf, sizeof(updated_time_buf), "%Y-%m-%d %H:%M:%S", updated_time_info);

            printf("%-5d %-24s %-8s %-19s %s\n",
                   items[i].id, items[i].title, items[i].author_id,
                   created_time_buf, updated_time_buf);
        }
        printf("---------------------------------------------------------------------------\n"); // Adjusted width
        
        int has_next_page = (actual == limit);

        printf("선택:\n");
        printf("  [글번호] → 해당 글 상세보기\n");
        printf("  [P] 이전 페이지\t [N] 다음 페이지\n");
        printf("  [S] 검색\n");
        printf("  [C] 글 작성\n");
        printf("  [R] 새로고침 (검색 초기화)\n");
        printf("  [B] 뒤로(게시판 메인으로)\n");
        printf("입력: ");

        if (fgets(line, sizeof(line), stdin) == NULL) return;
        // 개행 제거
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "B") == 0 || strcmp(line, "b") == 0) {
            return; // 게시판 메인으로
        } else if (strcmp(line, "R") == 0 || strcmp(line, "r") == 0) {
            page = 0;
            search_type[0] = '\0';
            keyword[0] = '\0';
            continue;
        } else if (strcmp(line, "P") == 0 || strcmp(line, "p") == 0) {
            if (page > 0) page--;
            continue;
        } else if (strcmp(line, "N") == 0 || strcmp(line, "n") == 0) {
            if (has_next_page) page++;
            continue;
        } else if (strcmp(line, "S") == 0 || strcmp(line, "s") == 0) {
            printf("검색 옵션:\n 1. 제목으로 검색\n 2. 작성자로 검색\n선택: ");
            if (fgets(line, sizeof(line), stdin) == NULL) continue;
            int search_opt = atoi(line);

            if (search_opt == 1) {
                strcpy(search_type, "title");
            } else if (search_opt == 2) {
                strcpy(search_type, "author");
            } else {
                printf("[안내] 잘못된 선택입니다.\n");
                printf("\n계속하려면 Enter 키를 누르세요...");
                fgets(line, sizeof(line), stdin);
                continue;
            }

            printf("검색어: ");
            if (fgets(keyword, sizeof(keyword), stdin) == NULL) {
                search_type[0] = '\0'; // Cancel search
                continue;
            }
            keyword[strcspn(keyword, "\n")] = '\0';
            if (keyword[0] == '\0') {
                search_type[0] = '\0'; // Cancel search if keyword is empty
                printf("[안내] 검색어가 비어있어 검색을 취소합니다.\n");
                printf("\n계속하려면 Enter 키를 누르세요...");
                fgets(line, sizeof(line), stdin);
                continue;
            }

            page = 0; // Reset to first page for new search
            continue;

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

// ===== 내 글 목록 화면 =====
static void board_screen_my_posts(ClientContext *ctx) {
    if (ctx->user_id[0] == '\0') {
        printf("[안내] 로그인이 필요합니다.\n");
        printf("\n계속하려면 Enter 키를 누르세요...");
        char line[BUF_SIZE];
        fgets(line, sizeof(line), stdin);
        return;
    }
    board_screen_list(ctx, ctx->user_id);
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
        printf("3. 내 글 목록\n");
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
                board_screen_list(ctx, NULL);
                break;

            case 2:
                board_screen_create(ctx);
                break;

            case 3:
                board_screen_my_posts(ctx);
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