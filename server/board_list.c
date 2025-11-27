#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"
#include "board_list.h"
#include "board_create.h"

#define MAX_POST 1000

void board_list(const char *db_path) {
    int fd;
    struct Board *post = malloc(MAX_POST * sizeof(struct Board));
    struct Board temp;
    int cnt = 0;

    if (!db_path) {
        fprintf(stderr, "[오류] DB 경로가 없습니다. \n");
        return;
    }

    if ((fd = open(db_path, O_RDONLY)) == -1) {
        perror("[오류] 게시판 파일 열기 실패");
        free(post);
        return;
    }

    // 파일에서 모든 게시글 읽기 <======== 나중에 n개씩 읽도록 수정 필요
    while (read(fd, &temp, sizeof(struct Board)) == sizeof(struct Board)) {
        if (cnt >= MAX_POST) {
            break;
        }
        post[cnt] = temp;
        cnt++;
    }
    close(fd);

    printf("===== 글 목록 =====\n");
    printf("%-6s %-20s %-10s\n", "글 번호", "제목", "작성자ID");

    if (cnt == 0) {
        printf("[안내] 등록된 게시글이 없습니다.\n");
        free(post);
        return;
    }

    //  최신 글부터 출력
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

    // 숫자 입력 시 해당 글 상세보기
    int post_num;
    char tmp;
    if (sscanf(input, "%d%c", &post_num, &tmp) == 1) {
        board_read(db_path, post_num);
        free(post);
        return;
    }

    // C: 글 작성
    if (strcmp(input, "C") == 0) {
        board_create(db_path);
        free(post);
        return;
    }

    // R: 새로고침
    if (strcmp(input, "R") == 0) {
        board_list(db_path);
        free(post);
        return;
    }

    // B: 뒤로 (게시판 메인으로)
    if (strcmp(input, "B") == 0) {
        printf("[안내] 게시판 메인으로 돌아갑니다. \n");
        free(post);
        return;
    }

    printf("[오류] 잘못된 입력입니다. \n");
    free(post);
    return;
}
