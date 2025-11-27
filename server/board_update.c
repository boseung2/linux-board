#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"
#include "board_update.h"

void board_update(const char *db_path, int target_id) {
    int fd;
    struct Board post;
    int found = 0;
    off_t pos = 0;

    if (!db_path) {
        fprintf(stderr, "[오류] DB 경로가 없습니다. \n");
        return;
    }

    if ((fd = open(db_path, O_RDWR)) == -1) {
        perror("[오류] 파일 열기 실패");
        return;
    }

    // 전체에서 수정할 글 찾기
    while (1) {
        pos = lseek(fd, 0, SEEK_CUR);

        if (read(fd, &post, sizeof(struct Board)) != sizeof(struct Board)) {
            break;
        }

        if (post.id == target_id) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("[오류] %d번 게시글을 찾을 수 없습니다.\n", target_id);
        close(fd);
        return;
    }
    if (post.is_deleted) {
        printf("[오류] 삭제된 글은 수정할 수 없습니다.\n");
        close(fd);
        return;
    }

    printf("===== 글 수정 =====\n");

    printf("기존 제목: %s\n", post.title);
    printf("새 제목 (Enter 시 기존 유지): ");

    char new_title[128];
    fgets(new_title, sizeof(new_title), stdin);
    new_title[strcspn(new_title, "\n")] = 0;

    if (strlen(new_title) > 0) {
        strncpy(post.title, new_title, TITLE_MAX);
    }

    printf("기존 내용: \n");
    printf("%s\n", post.content);

    printf("\n내용 수정 (입력 후 .만 단독으로 입력하면 종료): \n\n");

    char buffer[256];
    char full_content[CONTENT_MAX] = {0};
    int len = 0;

    while (1) {
        printf("> ");
        fgets(buffer, sizeof(buffer), stdin);

        if (strcmp(buffer, ".\n") == 0 || strcmp(buffer, ".") == 0) {
            break;
        }

        int need = strlen(buffer);
        if (len + need >= CONTENT_MAX) {
            printf("[경고] 내용이 너무 깁니다.\n");
            break;
        }

        strcat(full_content, buffer);
        len += need;
    }
    
    if (strlen(full_content) > 0) {
        strncpy(post.content, full_content, CONTENT_MAX);
    }

    printf("--------------------------------\n");
    printf("1. 수정 완료\n");
    printf("2. 취소\n");
    printf("선택: ");

    char sel_buf[8];
    fgets(sel_buf, sizeof(sel_buf), stdin);
    int sel = atoi(sel_buf);

    if (sel == 2) {
        printf("[취소] 수정이 취소되었습니다.\n");
        close(fd);
        return;
    }

    post.updated_at = time(NULL);

    lseek(fd, pos, SEEK_SET);
    if (write(fd, &post, sizeof(struct Board)) != sizeof(struct Board)) {
        perror("[오류] 쓰기 실패");
        return;
    }

    printf("[완료] %d번 게시글이 성공적으로 수정되었습니다.\n", target_id);
    close(fd);
    return;
}