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
    int author_id;             // 작성자 ID (user 구조체의 id와 매칭)

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

// ====== 초기화 함수 ======

// 서버 시작 시 1회 호출 (파일 생성/간단한 점검용)
void board_system_init(void);

int handle_board_command(int fd, const char *cmd, const char *args);

// ====== 게시글 CRUD ======

/**
 * 게시글 생성
 * author_id : 작성자 ID
 * title     : 제목 (NULL 아님)
 * content   : 전체 내용 (NULL 아님)
 * out_post  : 실제 저장된 게시글을 받고 싶으면 사용 (NULL 가능)
 *
 * return: BOARD_OK 또는 에러 코드
 */
int board_create_record(int author_id,
                        const char *title,
                        const char *content,
                        struct Board *out_post);

/**
 * id로 게시글 1건 조회
 * id       : 글 번호
 * out_post : 결과를 저장할 포인터 (NULL이면 의미 없음)
 *
 * is_deleted == 1인 글은 기본적으로 BOARD_ERR_NOT_FOUND 처리할지,
 * 그대로 넘길지는 구현 설계에 따라 결정.
 */
int board_get_by_id(int id, struct Board *out_post);

/**
 * 게시글 목록 조회
 * - offset, limit 기반 페이징
 * - is_deleted == 0 인 글만 반환하는 걸 기본 전략으로 가정
 *
 * out_array  : 결과를 담을 배열
 * max_count  : 배열 크기
 * out_count  : 실제로 채운 개수
 */
int board_list_range(int offset,
                     int limit,
                     struct Board *out_array,
                     int max_count,
                     int *out_count);

/**
 * 게시글 내용/제목 수정 (단순 버전)
 */
int board_update_record(int id,
                        const char *new_title,
                        const char *new_content);

/**
 * 게시글 삭제
 * - 실제 물리 삭제가 아니라 is_deleted = 1 로 soft delete하는 버전 추천.
 */
int board_soft_delete(int id);

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