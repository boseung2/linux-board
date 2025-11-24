#ifndef UI_AUTH_H
#define UI_AUTH_H

#include "client.h"

int ui_signup(ClientContext *ctx); // 성공 0, 실패/에러는 양수/음수
int ui_login(ClientContext *ctx);  // 성공 0, 실패/에러는 양수/음수

#endif