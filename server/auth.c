#include <unistd.h>
#include <string.h>

#include "auth.h"
#include "user.h"
#include "log.h"

int handle_signup(int fd, const char *id, const char *pw) {
    LOG_INFO("SIGNUP request (fd=%d, id=%s)", fd, id);

    int res = user_signup(id, pw);
    const char *msg;

    if (res == USER_OK) {
        msg = "OK SIGNUP\n";
        LOG_INFO("SIGNUP success (id=%s)", id);
    } else if (res == USER_ERR_EXISTS) {
        msg = "FAIL SIGNUP ID_EXISTS\n";
        LOG_WARN("SIGNUP failed: ID_EXISTS (id=%s)", id);
    } else if (res == USER_ERR_FULL) {
        msg = "FAIL SIGNUP SERVER_FULL\n";
        LOG_ERROR("SIGNUP failed: SERVER_FULL");
    } else {
        msg = "FAIL SIGNUP IO_ERROR\n";
        LOG_ERROR("SIGNUP failed: IO_ERROR (id=%s)", id);
    }

    write(fd, msg, strlen(msg));
    return res;
}

int handle_login(int fd, const char *id, const char *pw) {
    LOG_INFO("LOGIN request (fd=%d, id=%s)", fd, id);

    int res = user_login(id, pw);
    const char *msg;

    if (res == USER_OK) {
        msg = "OK LOGIN\n";
        LOG_INFO("LOGIN success (id=%s)", id);
    } else if (res == USER_ERR_NO_SUCH_ID) {
        msg = "FAIL LOGIN NO_SUCH_ID\n";
        LOG_WARN("LOGIN failed: NO_SUCH_ID (id=%s)", id);
    } else if (res == USER_ERR_WRONG_PW) {
        msg = "FAIL LOGIN WRONG_PASSWORD\n";
        LOG_WARN("LOGIN failed: WRONG_PASSWORD (id=%s)", id);
    } else {
        msg = "FAIL LOGIN UNKNOWN_ERROR\n";
        LOG_ERROR("LOGIN failed: UNKNOWN_ERROR (id=%s)", id);
    }

    write(fd, msg, strlen(msg));
    return res;
}

int handle_quit(int fd) {
    const char *msg = "BYE\n";
    LOG_INFO("QUIT request (fd=%d)", fd);
    write(fd, msg, strlen(msg));
    // 여기서 소켓을 바로 닫을지, 메인 루프에서 처리할지는 정책에 따라
    return 0;
}