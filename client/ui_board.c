// client/ui_board.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <wchar.h>
#include <locale.h>

#include "client.h"
#include "ui_board.h"
#include "socket.h"

// Function to get the display width of a string (considering multibyte characters)
static int get_display_width(const char* s) {
    int width = 0;
    mbstate_t mbs;
    wchar_t wc;
    const char *p = s;
    size_t len = strlen(s);

    memset(&mbs, 0, sizeof(mbs));
    while (len > 0) {
        size_t n = mbrtowc(&wc, p, len, &mbs);
        if (n == 0) break;
        if (n == (size_t)-1 || n == (size_t)-2) {
            width++;
            p++;
            len--;
            continue;
        }
        width += wcwidth(wc);
        p += n;
        len -= n;
    }
    return width;
}

// Function to print text with word wrapping
static void print_wrapped_text(const char *text, int max_line_width, const char *prefix) {
    mbstate_t mbs;
    const char *p = text;
    int current_line_width = 0;
    int prefix_width = get_display_width(prefix);

    memset(&mbs, 0, sizeof(mbs));
    printf("%s", prefix);
    current_line_width += prefix_width;

    while (*p) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, p, strlen(p), &mbs);

        if (n == 0) break;
        if (n == (size_t)-1 || n == (size_t)-2) {
            putchar(*p);
            current_line_width++;
            p++;
            continue;
        }
        
        if (strncmp(p, "<BR>", 4) == 0) {
            printf("\n");
            for(int i = 0; i < prefix_width; i++) printf(" ");
            current_line_width = prefix_width;
            p += 4;
            continue;
        }

        int char_width = wcwidth(wc);
        if (current_line_width + char_width > max_line_width) {
            printf("\n");
            for(int i = 0; i < prefix_width; i++) printf(" ");
            current_line_width = prefix_width;
        }

        for (size_t i = 0; i < n; i++) {
            putchar(p[i]);
        }
        current_line_width += char_width;
        p += n;
    }
    // No final newline here, to allow appending things like timestamp
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

    printf("제목: ");
    if (fgets(title, sizeof(title), stdin) == NULL) {
        return;
    }
    title[strcspn(title, "\n")] = '\0';

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

    char send_buf[BUF_SIZE];
    snprintf(send_buf, sizeof(send_buf), "POST %s|%s|%s\n", ctx->user_id, title, content);
    if (send_line(ctx->sock, send_buf) != 0) {
        printf("[오류] 서버로 데이터를 전송하지 못했습니다.\n");
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    char recv_buf[BUF_SIZE];
    if (read_line(ctx, ctx->sock, recv_buf, sizeof(recv_buf)) <= 0) {
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

    printf("기존 제목: %s\n", old_title);
    printf("새 제목(Enter 시 기존 제목 유지): ");
    if (fgets(input_title, sizeof(input_title), stdin) == NULL) {
        return;
    }
    input_title[strcspn(input_title, "\n")] = '\0';

    if (strlen(input_title) == 0) {
        strncpy(input_title, old_title, sizeof(input_title) - 1);
        input_title[sizeof(input_title) - 1] = '\0';
    }

    printf("기존 내용: \n");
    print_wrapped_text(old_content, 60, "");


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
    setlocale(LC_ALL, "");
    char line[BUF_SIZE];

    system("clear");
    printf("====== 글 상세 ======\n");

    char send_buf[64];
    snprintf(send_buf, sizeof(send_buf), "VIEW %d\n", post_id);
    if (send_line(ctx->sock, send_buf) != 0) {
        printf("[오류] 서버 전송 실패\n");
        printf("\n계속하려면 Enter 키를 누르세요...");
        fgets(line, sizeof(line), stdin);
        return;
    }

    if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) {
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

    if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    sscanf(line, "ID %d", &id);

    if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    sscanf(line, "AUTHOR %s", author_id);
    author_id[strcspn(author_id, "\n")] = '\0';

    if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    if (strncmp(line, "TITLE ", 6) == 0) {
        strncpy(title, line + 6, sizeof(title) - 1);
        title[strcspn(title, "\n")] = '\0';
    }

    if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    sscanf(line, "DATE %ld", &epoch);

    if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) goto read_error;
    if (strncmp(line, "CONTENT", 7) != 0) {
        printf("[오류] 프로토콜 에러: CONTENT 헤더 없음\n");
        goto wait_enter;
    }

    char content[2048];
    content[0] = '\0';
    size_t len = 0;

    while (1) {
        if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) break;
        if (strcmp(line, ".\n") == 0 || strcmp(line, ".") == 0 || strcmp(line, ".\r\n") == 0) break;

        size_t need = strlen(line);
        if (len + need >= sizeof(content) - 1) {
            break;
        }
        memcpy(content + len, line, need);
        len += need;
        content[len] = '\0';
    }

    char time_buf[20];
    time_t raw_time = (time_t)epoch;
    struct tm *time_info = localtime(&raw_time);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", time_info);

    printf("글번호: %d\n", id);
    printf("제목: %s\n", title);
    printf("작성자: %s\n", author_id);
    printf("작성일: %s\n", time_buf);
    printf("--------------------------------\n");
    printf("내용:\n");

    print_wrapped_text(content, 60, "");
    printf("\n");


    printf("--------------------------------\n");

    if (read_line(ctx, ctx->sock, line, sizeof(line)) > 0 && strncmp(line, "COMMENTS", 8) == 0) {
        int comment_count;
        sscanf(line, "COMMENTS %d", &comment_count);
        printf("\n====== 댓글 (%d) ======\n", comment_count);

        for (int i = 0; i < comment_count; i++) {
            if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) break;
            int c_id;
            char c_author[32];
            long c_epoch;
            char c_content[256];

            sscanf(line, "%d|%31[^|]|%ld|%255[^\n]", &c_id, c_author, &c_epoch, c_content);
            
            char c_time_buf[20];
            time_t c_raw_time = (time_t)c_epoch;
            struct tm *c_time_info = localtime(&c_raw_time);
            strftime(c_time_buf, sizeof(c_time_buf), "%Y-%m-%d %H:%M", c_time_info);

            char comment_prefix[64];
            snprintf(comment_prefix, sizeof(comment_prefix), "[%s] ", c_author);
            print_wrapped_text(c_content, 60, comment_prefix);
            printf(" (%s)\n", c_time_buf);
        }
        printf("========================\n\n");
    }

    printf("1. 댓글 작성\n");
    printf("2. 글 수정\n");
    printf("3. 글 삭제\n");
    printf("4. 목록으로 돌아가기\n");
    printf("선택: ");

    if (fgets(line, sizeof(line), stdin) == NULL) return;
    int sel = atoi(line);

    if (sel == 1) {
        char comment_content[256];
        printf("댓글 내용: ");
        if (fgets(comment_content, sizeof(comment_content), stdin) == NULL) return;
        comment_content[strcspn(comment_content, "\n")] = '\0';

        char comment_req[BUF_SIZE];
        snprintf(comment_req, sizeof(comment_req), "COMMENT %d|%s\n", id, comment_content);
        if (send_line(ctx->sock, comment_req) != 0) {
            printf("\n[오류] 댓글 전송 실패\n");
        } else {
            char comment_resp[BUF_SIZE];
            if (read_line(ctx, ctx->sock, comment_resp, sizeof(comment_resp)) > 0) {
                if (strncmp(comment_resp, "OK COMMENT", 10) == 0) {
                    printf("\n[완료] 댓글이 작성되었습니다.\n");
                    board_screen_detail(ctx, id);
                    return;
                } else {
                    printf("\n[실패] 댓글 작성 실패: %s", comment_resp);
                }
            }
        }
        goto wait_enter;
    }

    else if (sel == 2) {
        char perm_req[64]; 
        snprintf(perm_req, sizeof(perm_req), "CHKPRM %d|UPDATE\n", id);

        if (send_line(ctx->sock, perm_req) != 0) {
            printf("\n[오류] 권한 요청 전송 실패\n");
            goto wait_enter;
        }

        char perm_resp[BUF_SIZE];
        if (read_line(ctx, ctx->sock, perm_resp, sizeof(perm_resp)) <= 0) {
            printf("\n[오류] 권한 응답 없음\n");
            goto wait_enter;
        }

        if (strncmp(perm_resp, "OK CHKPRM GRANTED", strlen("OK CHKPRM GRANTED")) != 0) {
            printf("\n[실패] 수정 권한이 없습니다.\n");
            printf("응답: %s", perm_resp);
            goto wait_enter;
        }

        char new_title[128];
        char new_content[2048];
        board_screen_update(ctx, id, title, content, new_title, new_content);

        char send_upd[BUF_SIZE];
        snprintf(send_upd, sizeof(send_upd), "UPDATE %d|%s|%s\n", id, new_title, new_content);
        if (send_line(ctx->sock, send_upd) != 0) {
            printf("[오류] 수정 요청 전송 실패\n");
        } else {
            char resp[BUF_SIZE];
            if (read_line(ctx, ctx->sock, resp, sizeof(resp)) > 0) {
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

    else if (sel == 3) {
        char perm_req[64];
        snprintf(perm_req, sizeof(perm_req), "CHKPRM %d|DELETE\n", id);

        if (send_line(ctx->sock, perm_req) != 0) {
            printf("\n[오류] 권한 요청 전송 실패\n");
            goto wait_enter;
        }

        char perm_resp[BUF_SIZE];
        if (read_line(ctx, ctx->sock, perm_resp, sizeof(perm_resp)) <= 0) {
            printf("\n[오류] 권한 응답 없음\n");
            goto wait_enter;
        }

        if (strncmp(perm_resp, "OK CHKPRM GRANTED", strlen("OK CHKPRM GRANTED")) != 0) {
            printf("\n[실패] 삭제 권한이 없습니다.\n");
            printf("응답: %s", perm_resp);
            goto wait_enter;
        }

        char YN[8];
        printf("정말 이 글을 삭제하시겠습니까? (Y/N): ");
        if (fgets(YN, sizeof(YN), stdin) == NULL) goto wait_enter;
        if (YN[0] != 'Y' && YN[0] != 'y') {
            printf("[취소] 글 삭제가 취소되었습니다.\n");
            goto wait_enter;
        }

        char send_del[64];
        snprintf(send_del, sizeof(send_del), "DELETE %d\n", id);
        if (send_line(ctx->sock, send_del) != 0) {
            printf("[오류] 삭제 요청 전송 실패\n");
        } else {
            char resp[BUF_SIZE];
            if (read_line(ctx, ctx->sock, resp, sizeof(resp)) > 0) {
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
    else if (sel == 4) {
        return;
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
    setlocale(LC_ALL, "");
    char line[BUF_SIZE];
    int page = 0;
    const int limit = 10;
    char search_type[32] = {0};
    char keyword[128] = {0};

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

        if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) {
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

        struct {
            int id;
            char title[128];
            char author_id[32];
            long created_at;
            long updated_at;
        } items[100];

        int actual = 0;
        for (int i = 0; i < count && i < 100; i++) {
            if (read_line(ctx, ctx->sock, line, sizeof(line)) <= 0) {
                return;
            }
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

        printf(" %-5s | %-30s | %-10s | %s\n", "번호", "제목", "작성자", "작성일");
        printf("-------+--------------------------------+------------+---------------------\n");
        for (int i = 0; i < actual; i++) {
            char created_time_buf[20];
            time_t raw_created_time = (time_t)items[i].created_at;
            struct tm *created_time_info = localtime(&raw_created_time);
            strftime(created_time_buf, sizeof(created_time_buf), "%Y-%m-%d %H:%M:%S", created_time_info);

            char truncated_title[128];
            int padding;
            int title_width = get_display_width(items[i].title);
            int max_title_width = 30;

            if (title_width > max_title_width) {
                int len = 0;
                int current_width = 0;
                mbstate_t mbs;
                memset(&mbs, 0, sizeof(mbs));
                const char* p = items[i].title;
                while(p[len] != '\0') {
                    wchar_t wc;
                    size_t n = mbrtowc(&wc, &p[len], strlen(&p[len]), &mbs);
                    if (n <= 0) break;
                    int char_width = wcwidth(wc);
                    if (current_width + char_width > max_title_width - 3) break;
                    current_width += char_width;
                    len += n;
                }
                strncpy(truncated_title, items[i].title, len);
                truncated_title[len] = '\0';
                strcat(truncated_title, "...");
                padding = max_title_width - (current_width + 3);
            } else {
                strcpy(truncated_title, items[i].title);
                padding = max_title_width - title_width;
            }
            if(padding < 0) padding = 0;

            printf(" %-5d | %s%*s | %-10s | %s\n",
                   items[i].id, 
                   truncated_title, 
                   padding, "",
                   items[i].author_id,
                   created_time_buf
                   );
        }
        printf("-------+--------------------------------+------------+---------------------\n");
        
        int has_next_page = (actual == limit);

        printf("\n선택:\n");
        printf("  [글번호] → 해당 글 상세보기\n");
        printf("  [P] 이전 페이지\t [N] 다음 페이지\n");
        printf("  [S] 검색\n");
        printf("  [C] 글 작성\n");
        printf("  [R] 새로고침 (검색 초기화)\n");
        printf("  [B] 뒤로(게시판 메인으로)\n");
        printf("입력: ");

        if (fgets(line, sizeof(line), stdin) == NULL) return;
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "B") == 0 || strcmp(line, "b") == 0) {
            return;
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
                search_type[0] = '\0';
                continue;
            }
            keyword[strcspn(keyword, "\n")] = '\0';
            if (keyword[0] == '\0') {
                search_type[0] = '\0';
                printf("[안내] 검색어가 비어있어 검색을 취소합니다.\n");
                printf("\n계속하려면 Enter 키를 누르세요...");
                fgets(line, sizeof(line), stdin);
                continue;
            }

            page = 0;
            continue;

        } else if (strcmp(line, "C") == 0 || strcmp(line, "c") == 0) {
            board_screen_create(ctx);
        } else {
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
                if (ctx->screen != SCREEN_BOARD) return;
                break;

            case 2:
                board_screen_create(ctx);
                if (ctx->screen != SCREEN_BOARD) return;
                break;

            case 3:
                board_screen_my_posts(ctx);
                if (ctx->screen != SCREEN_BOARD) return;
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
