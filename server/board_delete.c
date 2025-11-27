#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"
#include "board_delete.h"

void board_delete(const char *db_path, int target_id) {
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

    // 전체에서 삭제할 글 찾기
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
        printf("[안내] %d번 게시글은 이미 삭제된 상태입니다.\n", target_id);
        close(fd);
        return;
    }

    // 삭제 확인
    printf("정말 %d번 글을 삭제하시겠습니까?(Y/N): ", target_id);
    char c;
    scanf(" %c", &c);

    if (c == 'N') {
        printf("[취소] 삭제를 취소했습니다. \n");
        close(fd);
        return;
    }

    // 삭제 처리
    post.is_deleted = 1;
    post.updated_at = time(NULL);

    lseek(fd, pos, SEEK_SET);
    if (write(fd, &post, sizeof(struct Board)) != sizeof(struct Board)) {
        perror("[오류] 쓰기 실패");
        close(fd);
        return;
    }

    printf("[완료] %d번 게시글이 삭제되었습니다.\n", target_id);
    close(fd);
    return;
}
