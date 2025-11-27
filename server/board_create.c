#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"
#include "board_create.h"

#define MAX_LINES 20

// 다음 post_ID 계산
int nxt_id(int fd) {
    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        perror("lseek");
        return -1;
    }
    return (size / sizeof(struct Board)) + 1;
}

void board_create(const char *db_path) {
    int fd;
    struct Board post;

    if (!db_path) {
        fprintf(stderr, "[오류] DB 경로가 없습니다. \n");
        return;
    }

    if ((fd = open(db_path, O_WRONLY|O_CREAT|O_APPEND, 0640)) == -1) {
        perror(db_path);
        exit(2);
    }

    memset(&post, 0, sizeof(post));
    post.id = nxt_id(fd);
    post.author_id = 1; // 임시로 1번 사용자로 설정

    printf("====== 글 작성 ======\n");

    // 제목 입력
    printf("제목: ");
    fgets(post.title, TITLE_MAX, stdin);
    post.title[strcspn(post.title, "\n")] = 0;

    // 내용 입력
    printf("내용 (입력 후 .만 단독으로 입력하면 종료):\n\n");

    char buffer[256];
    char full_content[CONTENT_MAX] = {0};
    int len = 0;

    while (1) {
        printf("> ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }

        // 종료 조건: 단독 "." 입력
        if (strcmp(buffer, ".\n") == 0 || strcmp(buffer, ".") == 0)
            break;

        // 내용 이어붙이기 (용량 체크)
        int need = strlen(buffer);
        if (len + need >= CONTENT_MAX) {
            printf("[경고] 내용이 너무 깁니다. 더 이상 입력할 수 없습니다.\n");
            break;
        }

        strcat(full_content, buffer);
        len += need;
    }

    // 내용 복사 ((저장X
    strncpy(post.content, full_content, CONTENT_MAX);

    printf("--------------------------------\n");
    printf("1. 등록\n");
    printf("2. 취소\n");
    printf("선택: ");

    char sel_buf[8];
    fgets(sel_buf, sizeof(sel_buf), stdin);
    int sel = atoi(sel_buf);

    if (sel == 2) {
        printf("[취소] 글 작성이 취소되었습니다.\n");
        close(fd);
        return;
    }

    // 글 저장
    post.created_at = time(NULL);
    post.updated_at = post.created_at;
    post.is_notice = 0;
    post.Visibility = VISIBILITY_PUBLIC;
    post.view_count = 0;
    post.like_count = 0;
    post.comment_count = 0;

    memset(post.comment, 0, sizeof(post.comment));
    post.is_deleted = 0;

    // 파일 끝으로 이동 후 쓰기
    lseek(fd, 0, SEEK_END);

    if (write(fd, &post, sizeof(struct Board)) != sizeof(struct Board)) {
        perror("write");
        close(fd);
        return;
    }

    printf("[완료] %d번 게시글이 등록되었습니다.\n", post.id);
    close(fd);
}
