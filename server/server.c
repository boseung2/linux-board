#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../type/board.h"
#include "board_create.h"
#include "board_list.h"
#include "board_delete.h"

#define DB_PATH "board.db"

int main() {
    int sel;
    struct Board post;

    while(1) {
        system("clear");
        printf("===== 게시판 메인 =====\n");
        printf("[로그인 : %d]\n\n", post.author_id);

        printf("1. 글 목록 보기 \n");
        printf("2. 글 작성 \n");
        printf("3. 내 글 목록 \n");
        printf("4. 글 삭제 \n");
        printf("5. 로그아웃 \n");
        printf("6. 프로그램 종료 \n\n");

        printf("선택: ");
        scanf("%d", &sel);
        getchar(); // 개행 문자 제거

        printf("\n");

        switch (sel) {
            case 1:
                board_list(DB_PATH);
                break;

            case 2:
                board_create(DB_PATH);
                break;

            case 3:
                printf("[안내] 내 글 목록 기능은 아직 구현되지 않았습니다.\n");
                break;

            case 4: 
                int target_id;
                printf("삭제할 글 번호 입력: ");
                scanf("%d", &target_id);
                getchar(); // 개행 문자 제거
                board_delete(DB_PATH, target_id);
                break;
            
            case 5:
                printf("[안내] 로그아웃 기능은 아직 구현되지 않았습니다.\n");
                break;

            case 6:
                printf("프로그램을 종료합니다.\n");
                return 0;

            default:
                printf("잘못된 선택입니다. 다시 시도하세요.\n");
                break;
        }

        printf("\n계속하려면 Enter 키를 누르세요...");
        getchar();
    }
    return 0;
}