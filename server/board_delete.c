#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "../type/board.h"

int main(int argc, char *argv[]) {
    int fd;
    struct Board post;
    int found = 0, target_id = atoi(argv[2]);
    off_t position = 0;

    if (argc < 3) {
        fprintf(stderr, "사용법: %s board.db 글번호 \n", argv[0]);
        exit(1);
    }

    if ((fd = open(argv[1], O_RDWR)) == -1) {
        perror("open");
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
        printf("[안내] %d번 게시글은 이미 삭제된 상태입니다.\n", target_id);
        close(fd);
        return 0;
    }

    printf("정말 %d번 글을 삭제하시겠습니까?(Y/N): ", target_id);
    char c;
    scanf(" %c", &c);

    if (c=='N') {
        printf("[취소] 삭제를 취소했습니다.\n");
        close(fd);
        return 0;
    }

    post.is_deleted = 1;
    post.updated_at = time(NULL);

    lseek(fd, position, SEEK_SET);
    if (write(fd, &post, sizeof(struct Board)) != sizeof(struct Board)) {
        perror("write");
        close(fd);
        exit(3);
    }

    printf("[완료] %d번 게시글이 삭제되었습니다.\n", target_id);
    close(fd);
    return 0;
}
