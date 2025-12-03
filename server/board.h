#ifndef BOARD_H
#define BOARD_H

#include <time.h>

// ====== 기본 상수/에러 코드 ======

#define BOARD_OK        0
#define BOARD_ERR_IO    1
#define BOARD_ERR_ARG   2
#define BOARD_ERR_NOT_FOUND 3
#define BOARD_ERR_FULL  4   // 필요시

#define TITLE_MAX    64
#define CONTENT_MAX  2048
#define MAX_COMMENT  100
#define AUTHOR_ID_MAX 32

// ====== 구조체/타입 ======

// 공개 범위
typedef enum {
    VISIBILITY_PUBLIC = 0,   // 전체 공개
    VISIBILITY_FRIENDS = 1,  // 친구에게만
    VISIBILITY_PRIVATE = 2   // 나만 보기
} Visibility;

// 댓글
struct Comment {
    int id;
    int author_id;
    char content[256];
    time_t created_at;
};

// 게시글
struct Board {
    int id;                    // 글 번호 (1, 2, 3...)
    char author_id[AUTHOR_ID_MAX];             // 작성자 ID (user 구조체의 id와 매칭)

    char title[TITLE_MAX];     // 제목
    char content[CONTENT_MAX]; // 내용

    time_t created_at;         // 작성 시간
    time_t updated_at;         // 수정 시간

    int is_notice;             // 공지 여부 (0: 일반, 1: 공지)
    Visibility Visibility;     // 공개 범위

    int view_count;            // 조회수
    int like_count;            // 좋아요 수

    int comment_count;         // 댓글 개수
    struct Comment comment[MAX_COMMENT];  // 댓글 배열

    int is_deleted;            // 0: 활성, 1: 삭제됨 (soft delete)
};

// ====== 댓글 관련 (추가 예정이면) ======

/**
 * 댓글 추가 (필요하면 나중에 구현)
 */
int board_add_comment(int post_id,
                      int author_id,
                      const char *content);

/**
 * 댓글 삭제 등등...
 * int board_delete_comment(int post_id, int comment_id);
 */

#endif