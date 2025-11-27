#include <stdio.h>
#include <stdlib.h>

#include "../type/board.h"

int main() {
    int sel;
    struct Board post;

    system("clear");
    printf("===== 게시판 메인 =====\n");
    printf("[로그인 : %d]", post.author_id);

    printf("1. 글 목록 보기 \n");
    printf("2. 글 작성 \n");
    printf("3. 내 글 목록 \n");
    printf("4. 로그아웃 \n");
    printf("5. 프로그램 종료 \n\n");

    printf("선택: ");
    scanf("%d", &sel);

    printf("%d번 메뉴를 선택했습니다.\n", sel);
}