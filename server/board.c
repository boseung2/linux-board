#include "board.h"
#include "log.h"
#include <unistd.h>
#include <string.h>

// 예: POST, LIST, READ 같은 명령을 여기서 처리할 예정
int handle_board_command(int fd, const char *cmd, const char *args) {
    LOG_INFO("BOARD command (fd=%d, cmd=%s, args=%s)", fd, cmd, args ? args : "");

    const char *msg = "BOARD COMMAND NOT IMPLEMENTED YET\n";
    write(fd, msg, strlen(msg));
    return 0;
}