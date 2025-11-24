#ifndef UI_MAIN_MENU_H
#define UI_MAIN_MENU_H

#include "client.h"

// 메인 메뉴 화면 출력 + 입력 처리
void ui_main_menu_show();
int ui_main_menu_handle_input(ClientContext *ctx);

#endif