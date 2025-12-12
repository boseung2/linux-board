#include <unistd.h>
#include <string.h>

#include "auth.h"
#include "user.h"
#include "log.h"

static struct Session sessions[MAX_SESSION];


void session_init() {
    for (int i = 0; i < MAX_SESSION; i++) {
        sessions[i].fd = -1;
        sessions[i].logged_in = 0;
    }
}

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
        session_set(fd, id);
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

void session_set(int fd, const char *user_id) {
    for (int i = 0; i < MAX_SESSION; i++) {
        if (sessions[i].fd == -1 || sessions[i].fd == fd) {
            sessions[i].fd = fd;
            strcpy(sessions[i].user_id, user_id);
            sessions[i].logged_in = 1;
            LOG_INFO("Session set: fd=%d, user_id=%s", fd, user_id);
            return;
        }
    }
    LOG_WARN("Session set failed: no available slot for fd=%d", fd);
}

const char* session_get_user_id(int fd) {
    for (int i = 0; i < MAX_SESSION; i++) {
        if (sessions[i].fd == fd && sessions[i].logged_in) {
            return sessions[i].user_id;
        }
    }
    return NULL;
}

void session_logout(int fd) {
    for (int i = 0; i < MAX_SESSION; i++) {
        if (sessions[i].fd == fd) {
            LOG_INFO("Session logout: fd=%d, user_id=%s", fd, sessions[i].user_id);
            sessions[i].logged_in = 0;
            return;
        }
    }
    LOG_WARN("Session logout failed: no session found for fd=%d", fd);
}