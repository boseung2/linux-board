#ifndef USER_H
#define USER_H

#define MAX_USERS 1000

// 에러/결과 코드
#define USER_OK 0
#define USER_ERR_EXISTS 1
#define USER_ERR_FULL 2
#define USER_ERR_IO 3
#define USER_ERR_NO_SUCH_ID 4
#define USER_ERR_WRONG_PW 5

typedef struct {
  char id[32];
  char pw[32];
} User;

// user 시스템 초기화 (파일에서 기존 유저 로드)
void user_system_init(void);

// 회원가입: 성공이면 USER_OK, 실패면 위의 에러 코드 중 하나 리턴
int user_signup(const char *id, const char *pw);

// 로그인 체크: USER_OK / USER_ERR_NO_SUCH_ID / USER_ERR_WRONG_PW
int user_login(const char *id, const char *pw);

#endif