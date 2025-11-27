#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"

// int main(int argc, char *argv[]) {
//     int fd, id, withdraw;
//     char c;
//     struct account record;

//     if (argc < 2) {
//         fprintf(stderr, "사용법 : %s file\n", argv[0]);
//         exit(1);
//     }

//     if ((fd = open(argv[1], O_RDWR)) == -1) {
//         perror(argv[1]);
//         exit(2);
//     }

//     do {
//         printf("Account ID: ");
//         if (scanf("%d", &id) == 1) {
//             lseek(fd, (long)(id - START_ID) * sizeof(record), SEEK_SET);
//             if ((read(fd, &record, sizeof(record)) > 0) && (record.id != 0)) {
//                 printf("ID: %8d\t Name: %4s\t Balance: %4d\n", record.id, record.name, record.balance);
//                 printf("Amount of money to withdraw: ");
//                 scanf("%d", &withdraw);

//                 if (withdraw > 0)
//                 {
//                     if (withdraw > record.balance)
//                     {
//                         printf("should be smaller than %d\n", record.balance);
//                     }
//                     else
//                     {
//                         record.balance -= withdraw;
//                     }
//                 }
//                 else
//                 {
//                     record.balance += (-withdraw);
//                 }
//                 printf("Balance: %d\n", record.balance);
//                 lseek(fd, (long)-sizeof(record), SEEK_CUR);
//                 write(fd, &record, sizeof(record));
//             }
//             else {
//                 printf("레코드 %d 없음\n", id);
//             }
//         }
//         else {
//             printf("입력오류 \n");
//         }
//         printf("계속하겠습니까?(Y/N)");
//         scanf(" %c", &c);
//     } while (c == 'Y');
//     close(fd);
//     exit(0);
// }

int main(int argc, char *argv[]) {
    int fd;
    struct Board post;
    int found = 0, target_id = atoi(argv[2]);
    off_t position = 0;

    if (argc < 3) {
        fprintf(stderr, "사용법: %s board.db 글번호\n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDWR)) == -1) {
        perror(argv[1]);
        exit(2);
    }

    while (1) {
        position = lseek(fd, 0, SEEK_CUR);
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
        return 0;
    }
    if (post.is_deleted) {
        printf("[오류] 삭제된 글은 수정할 수 없습니다.\n");
        close(fd);
        return 0;
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

    int sel;
    scanf("%d", &sel);

    if (sel == 2) {
        printf("[취소] 수정이 취소되었습니다.\n");
        close(fd);
        return 0;
    }

    post.updated_at = time(NULL);
    lseek(fd, position, SEEK_SET);
    if (write(fd, &post, sizeof(struct Board)) != sizeof(struct Board)) {
        perror("write");
        exit(3);
    }

    printf("[완료] %d번 게시글이 성공적으로 수정되었습니다.\n", target_id);
    close(fd);
    return 0;
}