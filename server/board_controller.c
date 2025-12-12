#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "board.h"
#include "board_service.h"
#include "board_controller.h"
#include "auth.h"

/**
 * 클라이언트 명령 핸들러:
 *
 *  - POST  <author_id>|<title>|<content>
 *      → 게시글 생성
 *
 *  - LIST  [offset] [limit]
 *      → 목록 조회
 *      → 응답:
 *          OK LIST <count>\n
 *          <id>|<title>|<author_id>\n ... (count 줄)
 *
 *  - VIEW  <id>
 *      → 상세 조회
 *      → 응답:
 *          OK VIEW\n
 *          ID <id>\n
 *          AUTHOR <author_id>\n
 *          TITLE <title>\n
 *          DATE <epoch>\n
 *          CONTENT\n
 *          <내용 여러 줄>\n
 *          .\n   (CONTENT 종료 마커)
 *
 *  - DELETE <id>
 *      → soft delete
 *
 * server.c 에서:
 *   else if (strcmp(cmd, "POST") == 0 || strcmp(cmd, "LIST") == 0 ||
 *            strcmp(cmd, "VIEW") == 0 || strcmp(cmd, "DELETE") == 0) {
 *       const char *args = (n >= 2) ? line + (cmdlen + 1) : "";
 *       handle_board_command(fd, cmd, args);
 *   }
 */
int handle_board_command(int fd, const char *cmd, const char *args) {
    char send_buf[4096];

    // POST: "POST title|content"
    if (strcmp(cmd, "POST") == 0) {
        char title[TITLE_MAX];
        char content[CONTENT_MAX];
        char author_id[AUTHOR_ID_MAX];

        // 형식: "author_id|title|content"
        // 예: "boseung|오늘의일기|정말 피곤했다..."
        int n = sscanf(args, " %31[^|]|%63[^|]|%1023[^\n]", author_id, title, content);
        LOG_DEBUG("POST sscanf result: n=%d, author_id=%s, title='%s', content='%s'",
                  n, author_id, title, content);
        if (n < 3) {
            const char *msg = "FAIL POST INVALID_ARGS\n";
            write(fd, msg, strlen(msg));
            LOG_WARN("POST invalid args (fd=%d, args='%s')", fd, args ? args : "");
            return BOARD_ERR_ARG;
        }

        struct Board post;
        int res = board_create_record(author_id, title, content, &post);
        if (res == BOARD_OK) {
            snprintf(send_buf, sizeof(send_buf),
                     "OK POST %d\n", post.id);
            write(fd, send_buf, strlen(send_buf));
        } else {
            const char *msg = "FAIL POST INTERNAL_ERROR\n";
            write(fd, msg, strlen(msg));
        }
        return res;
    }

    // LIST: "LIST [offset] [limit] [search_type] [keyword]"
    else if (strcmp(cmd, "LIST") == 0) {
        int offset = 0;
        int limit  = 10;
        char search_type[32] = {0};
        char keyword[128] = {0};

        // sscanf will match as many as it can. Unmatched variables are not touched.
        int n = sscanf(args, "%d %d %31s %127[^\n]", &offset, &limit, search_type, keyword);

        if (offset < 0) offset = 0;
        if (limit <= 0) limit = 10;
        if (limit > 100) limit = 100;

        // Allocate 'posts' on the heap to avoid stack overflow
        struct Board *posts = malloc(sizeof(struct Board) * 100);
        if (posts == NULL) {
            LOG_ERROR("Failed to allocate memory for posts array");
            const char *msg = "FAIL LIST INTERNAL_ERROR\n";
            write(fd, msg, strlen(msg));
            return BOARD_ERR_IO; // Or a new memory error
        }
        
        int count = 0;
        
        const char *p_search_type = (n >= 3) ? search_type : NULL;
        const char *p_keyword = (n >= 4) ? keyword : NULL;

        LOG_DEBUG("Calling board_list_range with: offset=%d, limit=%d, n=%d, type='%s', keyword='%s'",
                  offset, limit, n, p_search_type ? p_search_type : "NULL", p_keyword ? p_keyword : "NULL");

        int res = board_list_range(offset, limit, posts, 100, &count, p_search_type, p_keyword);
        if (res != BOARD_OK) {
            const char *msg = "FAIL LIST INTERNAL_ERROR\n";
            write(fd, msg, strlen(msg));
            free(posts); // Don't forget to free
            return res;
        }

        // 첫 줄: OK LIST count
        snprintf(send_buf, sizeof(send_buf),
                 "OK LIST %d\n", count);
        write(fd, send_buf, strlen(send_buf));

        for (int i = 0; i < count; i++) {
            // 한 줄: "id|title|author_id|created_at_epoch|updated_at_epoch\n"
            snprintf(send_buf, sizeof(send_buf),
                     "%d|%s|%s|%ld|%ld\n",
                     posts[i].id, posts[i].title, posts[i].author_id,
                     (long)posts[i].created_at, (long)posts[i].updated_at);
            write(fd, send_buf, strlen(send_buf));
        }
        
        free(posts); // Free the memory
        return BOARD_OK;
    }

    // VIEW: "VIEW <id>" → 단일 게시글 상세
    else if (strcmp(cmd, "VIEW") == 0) {
        int id;
        int n = sscanf(args, "%d", &id);
        if (n < 1 || id <= 0) {
            const char *msg = "FAIL VIEW INVALID_ARGS\n";
            write(fd, msg, strlen(msg));
            LOG_WARN("VIEW invalid args (fd=%d, args='%s')", fd, args ? args : "");
            return BOARD_ERR_ARG;
        }

        struct Board post;
        int res = board_get_by_id(id, &post);
        if (res == BOARD_ERR_NOT_FOUND) {
            const char *msg = "FAIL VIEW NOT_FOUND\n";
            write(fd, msg, strlen(msg));
            return res;
        }
        if (res != BOARD_OK) {
            const char *msg = "FAIL VIEW INTERNAL_ERROR\n";
            write(fd, msg, strlen(msg));
            return res;
        }

        // OK VIEW 헤더
        snprintf(send_buf, sizeof(send_buf),
                 "OK VIEW\n");
        write(fd, send_buf, strlen(send_buf));

        // 메타정보 라인
        snprintf(send_buf, sizeof(send_buf),
                 "ID %d\nAUTHOR %s\nTITLE %s\nDATE %ld\n",
                 post.id, post.author_id, post.title, (long)post.created_at);
        write(fd, send_buf, strlen(send_buf));

        // CONTENT 블록 시작
        const char *content_header = "CONTENT\n";
        write(fd, content_header, strlen(content_header));

        // 내용 (그대로 전송, 마지막에 개행 보장)
        size_t clen = strlen(post.content);
        if (clen > 0) {
            write(fd, post.content, clen);
            if (post.content[clen - 1] != '\n') {
                write(fd, "\n", 1);
            }
        }

        // 종료 마커
        const char *end_marker = ".\n";
        write(fd, end_marker, strlen(end_marker));

        return BOARD_OK;
    }

    // DELETE: "DELETE <id>" → soft delete
    else if (strcmp(cmd, "DELETE") == 0) {
        int id;
        int n = sscanf(args, "%d", &id);
        if (n < 1 || id <= 0) {
            const char *msg = "FAIL DELETE INVALID_ARGS\n";
            write(fd, msg, strlen(msg));
            LOG_WARN("DELETE invalid args (fd=%d, args='%s')", fd, args ? args : "");
            return BOARD_ERR_ARG;
        }

        struct Board post;
        int r = board_load(id, &post);
        if (r != BOARD_OK) {
            const char *msg = "FAIL DELETE NOT_FOUND\n";
            write(fd, msg, strlen(msg));
            return BOARD_ERR_NOT_FOUND;
        }

        const char *user_id = session_get_user_id(fd);
        if (!user_id) user_id = "";

        if (strcmp(post.author_id, user_id) != 0) {
            const char *msg = "FAIL DELETE PERMISSION_DENIED\n";
            write(fd, msg, strlen(msg));
            LOG_WARN("DELETE permission denied (fd=%d, id=%d, user_id=%s)",
                     fd, post.id, user_id);
            return BOARD_ERR_PERMISSION;
        }

        // 권한 체크 후 실제 삭제 실행
        int res = board_soft_delete(id);
        if (res == BOARD_OK) {
            snprintf(send_buf, sizeof(send_buf),
                     "OK DELETE %d\n", id);
            write(fd, send_buf, strlen(send_buf));
        } else if (res == BOARD_ERR_NOT_FOUND) {
            const char *msg = "FAIL DELETE NOT_FOUND\n";
            write(fd, msg, strlen(msg));
        } else if (res == BOARD_ERR_PERMISSION) {
            const char *msg = "FAIL DELETE PERMISSION_DENIED\n";
            write(fd, msg, strlen(msg));
        } else {
            const char *msg = "FAIL DELETE INTERNAL_ERROR\n";
            write(fd, msg, strlen(msg));
        }
        return res;
    }

    // UPDATE: "UPDATE <id>|<new_title>|<new_content>"
    else if (strcmp(cmd, "UPDATE") == 0) {
        int id;
        char new_title[TITLE_MAX];
        char new_content[CONTENT_MAX];

        // 형식: "id|new_title|new_content"
        int n = sscanf(args, " %d|%63[^|]|%1023[^\n]", &id, new_title, new_content);
        LOG_DEBUG("UPDATE sscanf result: n=%d, id=%d, new_title='%s', new_content='%s'",
                  n, id, new_title, new_content);
        if (n < 3 || id <= 0) {
            const char *msg = "FAIL UPDATE INVALID_ARGS\n";
            write(fd, msg, strlen(msg));
            LOG_WARN("UPDATE invalid args (fd=%d, args='%s')", fd, args ? args : "");
            return BOARD_ERR_ARG;
        }

        struct Board post;
        int r = board_load(id, &post);
        if (r != BOARD_OK) {
            const char *msg = "FAIL UPDATE NOT_FOUND\n";
            write(fd, msg, strlen(msg));
            return BOARD_ERR_NOT_FOUND;
        }

        if (strcmp(post.author_id, session_get_user_id(fd)) != 0) {
            const char *msg = "FAIL UPDATE PERMISSION_DENIED\n";
            write(fd, msg, strlen(msg));
            LOG_WARN("UPDATE permission denied (fd=%d, id=%d, user_id=%s)",
                     fd, post.author_id, session_get_user_id(fd));
            return BOARD_ERR_PERMISSION;
        }

        // 업데이트 실행
        int res = board_update_record(id, new_title, new_content, session_get_user_id(fd));
        if (res == BOARD_OK) {
            snprintf(send_buf, sizeof(send_buf),
                     "OK UPDATE %d\n", id);
            write(fd, send_buf, strlen(send_buf));
        } else if (res == BOARD_ERR_NOT_FOUND) {
            const char *msg = "FAIL UPDATE NOT_FOUND\n";
            write(fd, msg, strlen(msg));
        } else {
            const char *msg = "FAIL UPDATE INTERNAL_ERROR\n";
            write(fd, msg, strlen(msg));
        }
        return res;
    }

    // 권한 체크(자신의 글만 수정/삭제)
    else if (strcmp(cmd, "CHKPRM") == 0) {
        int id;
        char action[32] = {0};

        // 형식: "id|ACTION"
        // int n = sscanf(args, " %d|%31s[^\n]", &id, action);
        int n = sscanf(args, " %d|%31s", &id, action);
        LOG_DEBUG("CHKPRM sscanf result: n=%d, id=%d, action='%s'",
                  n, id, action);
                  
        action[strcspn(action, "\r\n")] = '\0';
        if (n < 2 || id <= 0) {
            const char *msg = "FAIL CHKPRM INVALID_ARGS\n";
            write(fd, msg, strlen(msg));
            LOG_WARN("CHKPRM invalid args (fd=%d, args='%s')", fd, args ? args : "");
            return BOARD_ERR_ARG;
        }

        struct Board post;
        if (board_load(id, &post) < 0) {
            const char *msg = "FAIL CHKPRM NOT_FOUND\n";
            write(fd, msg, strlen(msg));
            return BOARD_ERR_NOT_FOUND;
        }

        // 로그인 된 user_id 가져오기
        const char *user_id = session_get_user_id(fd);
        if (user_id == NULL) user_id = "";
        
        // 작성자와 요청자가 같은지 확인(권한 설정)
        if (strcmp(post.author_id, user_id) != 0) {
            const char *msg = "FAIL CHKPRM DENIED\n";
            write(fd, msg, strlen(msg));
            LOG_WARN("CHKPRM permission denied (fd=%d, id=%d, user_id=%s)",
                     fd, post.author_id, session_get_user_id(fd));
            return BOARD_ERR_PERMISSION;
        }
        const char *msg = "OK CHKPRM GRANTED\n";
        write(fd, msg, strlen(msg));
        return BOARD_OK;
    }

    // 알 수 없는 게시판 명령
    else {
        const char *msg = "FAIL BOARD UNKNOWN_CMD\n";
        write(fd, msg, strlen(msg));
        LOG_WARN("Unknown board command (fd=%d, cmd=%s, args=%s)",
                 fd, cmd ? cmd : "(null)", args ? args : "");
        return BOARD_ERR_ARG;
    }
}
