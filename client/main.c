#include "client.h"
#include "socket.h"
#include "ui_main_menu.h"
#include "ui_auth.h"
#include "ui_board.h"

int main() {
    ClientContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.screen = SCREEN_MAIN_MENU;
    ctx.running = 1;

    if (net_connect(&ctx) == -1) {
        exit(1);
    }

    printf("Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);

    while (ctx.running) {
        switch (ctx.screen) {
            case SCREEN_MAIN_MENU:
                ui_main_menu_show();
                ui_main_menu_handle_input(&ctx);
                break;

            case SCREEN_LOGIN:
                ui_login(&ctx);
                break;

            case SCREEN_SIGNUP:
                ui_signup(&ctx);
                break;

            case SCREEN_BOARD:
                ui_board_main(&ctx);
                break;
        }
    }

    close(ctx.sock);
    return 0;
}