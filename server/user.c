#include "user.h"
#include <stdio.h>
#include <string.h>

#define USER_DB_PATH "data/users"

static User users[MAX_USERS];
static int user_count = 0;

// 내부용: ID로 인덱스 찾기
static int find_user_index(const char *id) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}

// 서버 시작 시 기존 유저 로드
void user_system_init(void) {
    FILE *fp = fopen(USER_DB_PATH, "r");
    if (fp == NULL) {
        // 파일이 없으면 새로 시작하는 거니까 그냥 리턴
        return;
    }

    char id[32], pw[32];
    while (fscanf(fp, "%31s %31s", id, pw) == 2) {
        if (user_count >= MAX_USERS) break;
        strcpy(users[user_count].id, id);
        strcpy(users[user_count].pw, pw);
        user_count++;
    }

    fclose(fp);
}

// 회원가입: 메모리에 추가 + 파일에 append
int user_signup(const char *id, const char *pw) {
    if (find_user_index(id) != -1) {
        return USER_ERR_EXISTS;
    }
    if (user_count >= MAX_USERS) {
        return USER_ERR_FULL;
    }

    // 메모리에 추가
    strcpy(users[user_count].id, id);
    strcpy(users[user_count].pw, pw);
    user_count++;

    // 파일에 append
    FILE *fp = fopen(USER_DB_PATH, "a");
    if (fp == NULL) {
        return USER_ERR_IO;
    }
    fprintf(fp, "%s %s\n", id, pw);
    fclose(fp);

    return USER_OK;
}

// 로그인 체크
int user_login(const char *id, const char *pw) {
    int idx = find_user_index(id);
    if (idx == -1) {
        return USER_ERR_NO_SUCH_ID;
    }
    if (strcmp(users[idx].pw, pw) != 0) {
        return USER_ERR_WRONG_PW;
    }
    return USER_OK;
}