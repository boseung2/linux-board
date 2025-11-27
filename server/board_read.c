#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"
#include "board_read.h"
#include "board_update.h"
#include "board_delete.h"

void board_read(const char *db_path, int target_id) {
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

    // 전체에서 읽을 글 찾기
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
        printf("[오류] %d번 게시글을 찾을 수 없습니다. \n", target_id);
        close(fd);
        return;
    }

    if (post.is_deleted) {
        printf("[안내] %d번 게시글은 삭제된 글입니다. \n", target_id);
        close(fd);
        return;
    }

    printf("===== 글 상세 =====\n");
    printf("글번호: %d\n", post.id);
    printf("제목: %s\n", post.title);
    printf("작성자: %d\n", post.author_id);
    printf("작성일: %s\n", ctime(&post.created_at));

    printf("--------------------------------\n");
    printf("내용: \n%s\n", post.content);
    printf("--------------------------------\n");

    printf("1. 글 수정 \n");
    printf("2. 글 삭제 \n");
    printf("3. 목록으로 돌아가기 \n");
    printf("선택: ");

    int sel;
    scanf("%d", &sel);
    printf("[선택] %d번 메뉴를 선택했습니다. \n", sel);

    if (sel == 1) {
        board_update(db_path, target_id);   
    }
    else if (sel == 2) {
        board_delete(db_path, target_id);
    }
    else {
        printf("[안내] 목록으로 돌아갑니다. \n");
    }
    close(fd);
    return;
}
