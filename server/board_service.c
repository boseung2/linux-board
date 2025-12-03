#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "board.h"
#include "board_service.h"

// 게시글 DB 파일 경로
#define BOARD_DB_PATH "data/boards"

// 내부 헬퍼: DB 파일 열기
static int open_board_db(int flags, mode_t mode) {
    int fd = open(BOARD_DB_PATH, flags, mode);
    if (fd == -1) {
        perror(BOARD_DB_PATH);
        LOG_ERROR("open_board_db failed: %s", BOARD_DB_PATH);
    }
    return fd;
}

// 내부 헬퍼: 다음 ID 계산
static int board_next_id(int fd) {
    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        perror("lseek");
        LOG_ERROR("board_next_id: lseek failed");
        return -1;
    }
    return (int)(size / sizeof(struct Board)) + 1;
}

// 시스템 초기화: 파일 존재 보장 정도만 수행
void board_system_init(void) {
    int fd = open_board_db(O_WRONLY | O_CREAT, 0640);
    if (fd == -1) {
        LOG_ERROR("board_system_init: cannot create/open db file");
        return;
    }
    close(fd);
    LOG_INFO("Board system initialized (db=%s)", BOARD_DB_PATH);
}

/**
 * 게시글 생성
 */
int board_create_record(const char *author_id,
                        const char *title,
                        const char *content,
                        struct Board *out_post)
{
    if (!title || !content) {
        LOG_WARN("board_create_record: NULL title/content");
        return BOARD_ERR_ARG;
    }

    int fd = open_board_db(O_RDWR | O_CREAT, 0640);
    if (fd == -1) {
        return BOARD_ERR_IO;
    }

    struct Board post;
    memset(&post, 0, sizeof(post));

    // 새 ID 계산
    int new_id = board_next_id(fd);
    if (new_id < 0) {
        close(fd);
        return BOARD_ERR_IO;
    }
    post.id = new_id;

    // author_id 복사 (길면 잘라냄)
    strncpy(post.author_id, author_id, AUTHOR_ID_MAX - 1);
    post.author_id[AUTHOR_ID_MAX - 1] = '\0';

    // 제목/내용 복사 (길면 잘라냄)
    strncpy(post.title, title, TITLE_MAX - 1);
    post.title[TITLE_MAX - 1] = '\0';

    strncpy(post.content, content, CONTENT_MAX - 1);
    post.content[CONTENT_MAX - 1] = '\0';

    // 메타데이터 기본값
    post.created_at   = time(NULL);
    post.updated_at   = post.created_at;
    post.is_notice    = 0;
    post.Visibility   = VISIBILITY_PUBLIC;
    post.view_count   = 0;
    post.like_count   = 0;
    post.comment_count = 0;
    memset(post.comment, 0, sizeof(post.comment));
    post.is_deleted   = 0;

    // 파일 끝으로 이동 후 기록
    if (lseek(fd, 0, SEEK_END) < 0) {
        perror("lseek");
        close(fd);
        return BOARD_ERR_IO;
    }

    ssize_t w = write(fd, &post, sizeof(struct Board));
    if (w != sizeof(struct Board)) {
        perror("write");
        LOG_ERROR("board_create_record: write failed (w=%zd)", w);
        close(fd);
        return BOARD_ERR_IO;
    }

    close(fd);

    LOG_INFO("Board created: id=%d, author_id=%s, title='%s'",
             post.id, post.author_id, post.title);

    if (out_post) {
        *out_post = post;

    }

    return BOARD_OK;
}

/**
 * ID로 게시글 1건 조회
 */
int board_get_by_id(int id, struct Board *out_post) {
    if (!out_post || id <= 0) return BOARD_ERR_ARG;

    int fd = open_board_db(O_RDONLY | O_CREAT, 0640);
    if (fd == -1) return BOARD_ERR_IO;

    struct Board post;
    int found = 0;

    // 파일 전체 순회
    while (1) {
        ssize_t r = read(fd, &post, sizeof(struct Board));
        if (r == 0) break;             // EOF
        if (r < 0) {
            perror("read");
            close(fd);
            return BOARD_ERR_IO;
        }
        if (r != sizeof(struct Board)) {
            LOG_WARN("board_get_by_id: partial record read (%zd)", r);
            break;
        }

        if (post.id == id && !post.is_deleted) {
            *out_post = post;
            found = 1;
            break;
        }
    }

    close(fd);

    if (!found) {
        return BOARD_ERR_NOT_FOUND;
    }
    return BOARD_OK;
}

/**
 * 게시글 목록 (offset/limit 기반)
 * - is_deleted == 0 인 글만 반환
 * - 최신 글이 먼저 보이도록 id 역순 정렬은 간단성을 위해 여기서는 하지 않고,
 *   저장된 순서대로 반환한다.
 */
int board_list_range(int offset,
                     int limit,
                     struct Board *out_array,
                     int max_count,
                     int *out_count)
{
    if (!out_array || !out_count || limit <= 0 || max_count <= 0) {
        return BOARD_ERR_ARG;
    }

    int fd = open_board_db(O_RDONLY | O_CREAT, 0640);
    if (fd == -1) return BOARD_ERR_IO;

    struct Board post;
    int skipped = 0;
    int filled  = 0;

    while (filled < max_count) {
        ssize_t r = read(fd, &post, sizeof(struct Board));
        if (r == 0) break; // EOF
        if (r < 0) {
            perror("read");
            close(fd);
            return BOARD_ERR_IO;
        }
        if (r != sizeof(struct Board)) {
            LOG_WARN("board_list_range: partial record read (%zd)", r);
            break;
        }

        if (post.is_deleted) continue;

        // offset 만큼 스킵
        if (skipped < offset) {
            skipped++;
            continue;
        }

        // limit 초과 시 종료
        if ((filled + 1) > limit) {
            break;
        }

        out_array[filled++] = post;
    }

    close(fd);
    *out_count = filled;
    return BOARD_OK;
}

/**
 * 게시글 수정 (제목/내용만)
 */
int board_update_record(int id,
                        const char *new_title,
                        const char *new_content)
{
    if (!new_title && !new_content) return BOARD_ERR_ARG;
    if (id <= 0) return BOARD_ERR_ARG;

    int fd = open_board_db(O_RDWR | O_CREAT, 0640);
    if (fd == -1) return BOARD_ERR_IO;

    struct Board post;
    int updated = 0;

    while (1) {
        off_t pos = lseek(fd, 0, SEEK_CUR);
        if (pos < 0) {
            perror("lseek");
            close(fd);
            return BOARD_ERR_IO;
        }

        ssize_t r = read(fd, &post, sizeof(struct Board));
        if (r == 0) break; // EOF
        if (r < 0) {
            perror("read");
            close(fd);
            return BOARD_ERR_IO;
        }
        if (r != sizeof(struct Board)) {
            LOG_WARN("board_update_record: partial record read (%zd)", r);
            break;
        }

        if (post.id == id && !post.is_deleted) {
            if (new_title) {
                strncpy(post.title, new_title, TITLE_MAX - 1);
                post.title[TITLE_MAX - 1] = '\0';
            }
            if (new_content) {
                strncpy(post.content, new_content, CONTENT_MAX - 1);
                post.content[CONTENT_MAX - 1] = '\0';
            }
            post.updated_at = time(NULL);

            // 커서 되돌리고 덮어쓰기
            if (lseek(fd, pos, SEEK_SET) < 0) {
                perror("lseek");
                close(fd);
                return BOARD_ERR_IO;
            }

            ssize_t w = write(fd, &post, sizeof(struct Board));
            if (w != sizeof(struct Board)) {
                perror("write");
                close(fd);
                return BOARD_ERR_IO;
            }

            updated = 1;
            break;
        }
    }

    close(fd);

    if (!updated) {
        return BOARD_ERR_NOT_FOUND;
    }
    return BOARD_OK;
}

/**
 * 게시글 삭제 (soft delete: is_deleted = 1)
 */
int board_soft_delete(int id) {
    if (id <= 0) return BOARD_ERR_ARG;

    int fd = open_board_db(O_RDWR | O_CREAT, 0640);
    if (fd == -1) return BOARD_ERR_IO;

    struct Board post;
    int deleted = 0;

    while (1) {
        off_t pos = lseek(fd, 0, SEEK_CUR);
        if (pos < 0) {
            perror("lseek");
            close(fd);
            return BOARD_ERR_IO;
        }

        ssize_t r = read(fd, &post, sizeof(struct Board));
        if (r == 0) break; // EOF
        if (r < 0) {
            perror("read");
            close(fd);
            return BOARD_ERR_IO;
        }
        if (r != sizeof(struct Board)) {
            LOG_WARN("board_soft_delete: partial record read (%zd)", r);
            break;
        }

        if (post.id == id && !post.is_deleted) {
            post.is_deleted = 1;

            if (lseek(fd, pos, SEEK_SET) < 0) {
                perror("lseek");
                close(fd);
                return BOARD_ERR_IO;
            }

            ssize_t w = write(fd, &post, sizeof(struct Board));
            if (w != sizeof(struct Board)) {
                perror("write");
                close(fd);
                return BOARD_ERR_IO;
            }

            deleted = 1;
            break;
        }
    }

    close(fd);

    if (!deleted) {
        return BOARD_ERR_NOT_FOUND;
    }
    LOG_INFO("Board soft-deleted: id=%d", id);
    return BOARD_OK;
}
