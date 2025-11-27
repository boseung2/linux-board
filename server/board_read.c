#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"

#define BOARD_DB "board.db"

int main(int argc, char *argv[]) {
    int fd;
    struct Board post;
    int found = 0;

    if (argc < 3) {
        fprintf(stderr, "사용법: %s board.db 글번호\n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror(argv[1]);
        exit(2);
    }

    int target_id = atoi(argv[2]);
    while (read(fd, &post, sizeof(struct Board)) == sizeof(struct Board)) {
        if (post.id == target_id) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("[오류] %d번 게시글을 찾을 수 없습니다. \n", target_id);
        close(fd);
        return 0;
    }

    if (post.is_deleted) {
        printf("[안내] %d번 게시글은 삭제된 글입니다. \n", target_id);
        close(fd);
        return 0;
    }

    char date_str[64];
    struct tm *tm_info = localtime(&post.created_at);
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", tm_info);

    printf("===== 글 상세 =====\n");
    printf("글번호: %d\n", post.id);
    printf("제목: %s\n", post.title);
    printf("작성자: %d\n", post.author_id);
    printf("작성일: %s\n", date_str);

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

    close(fd);
    return 0;
}
