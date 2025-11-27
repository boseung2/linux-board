#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"

#define BOARD_DB "board.db"
// 여러 줄 입력 받을 때 최대 라인 수
#define MAX_LINES 20

// 다음 ID 계산
int nxt_id(int fd) {
    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        perror("lseek");
        return -1;
    }
    return (size / sizeof(struct Board)) + 1;
}

int main(int argc, char *argv[]) {
    int fd;
    struct Board post;

    if (argc < 2) {
        fprintf(stderr, "사용법 : %s board.db\n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_WRONLY|O_CREAT, 0640)) == -1) {
        perror(argv[1]);
        exit(2);
    }

    memset(&post, 0, sizeof(post));
    post.id = nxt_id(fd);
    post.author_id = 1;

    printf("====== 글 작성 ======\n");

    // 제목 입력
    printf("제목: ");
    fgets(post.title, TITLE_MAX, stdin);
    post.title[strcspn(post.title, "\n")] = 0;

    // 내용 입력 안내
    printf("내용 (입력 후 .만 단독으로 입력하면 종료):\n\n");

    // 여러 줄 입력 처리
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

    // 내용 복사
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
        return 0;
    }

    // 글 등록
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
        exit(1);
    }

    printf("[완료] %d번 게시글이 등록되었습니다.\n", post.id);
    close(fd);
    return 0;
}
