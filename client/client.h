#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9000
#define BUF_SIZE 4096

typedef enum {
  SCREEN_MAIN_MENU, // 메인 화면
  SCREEN_LOGIN,     // 로그인 화면
  SCREEN_SIGNUP,    // 회원가입 화면
  SCREEN_BOARD      // 게시판 메인 화면 (로그인 성공 후)
} ScreenState;

// 클라이언트 전체 상태(공유 상태)
typedef struct {
  int sock;
  ScreenState screen;
  int running;
  char user_id[32];
} ClientContext;

#endif