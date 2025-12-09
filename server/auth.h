#ifndef AUTH_H
#define AUTH_H

#define AUTHOR_ID_MAX 32
#define MAX_SESSION 1024

struct Session {
    int fd;                         // 소켓 FD
    char user_id[AUTHOR_ID_MAX];    // 로그인한 사용자 ID
    int logged_in;                  // 1: 로그인, 0: 비로그인
};

int handle_signup(int fd, const char *id, const char *pw);
int handle_login(int fd, const char *id, const char *pw);
int handle_quit(int fd);

void session_init();
void session_set(int fd, const char *user_id);
const char* session_get_user_id(int fd);
void session_logout(int fd);

#endif