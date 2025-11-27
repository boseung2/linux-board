#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"
#define MAX_POST 1000

int main(int argc, char *argv[]) {
    int fd;
    struct Board *post = malloc(MAX_POST * sizeof(struct Board));
    struct Board temp;
    int cnt = 0;

    if (argc < 2) {
        fprintf(stderr, "사용법: %s board.db\n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
        perror("open");
        exit(2);
    }

    while (read(fd, &temp, sizeof(struct Board)) == sizeof(struct Board)) {
        if (cnt >= MAX_POST) {
            break;
        }
        post[cnt] = temp;
        cnt++;
    }
    close(fd);

    printf("===== 글 목록 =====\n");
    printf("%-6s %-20s %-10s\n", "글 번호", "제목", "작성자");
    if (cnt == 0) {
        printf("[안내] 등록된 게시글이 없습니다.\n");
        return 0;
    }

    for (int i = cnt - 1; i >= 0; i--) {
        struct Board *p = &post[i];

        if (p->is_deleted) {
            continue;
        }

        char cur_post_title[21];
        strncpy(cur_post_title, p->title, 20);
        cur_post_title[20] = '\0';

        printf("%-6d %-20s %-10d\n", p->id, cur_post_title, p->author_id);
    }

    printf("-------------------------------------------\n");
    printf("선택: \n");
    printf("   [글 번호 입력] 해당 글 상세보기 \n");
    printf("   [C] 글 작성 \n");
    printf("   [R] 새로고침 \n");
    printf("   [B] 뒤로(게시판 메인으로) \n");
    printf("\n입력: ");

    char input[16];
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;

    printf("[입력] %s\n", input);
    free(post);
    return 0;
}
