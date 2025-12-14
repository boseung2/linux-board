#ifndef NET_H
#define NET_H

#include "client.h"

int net_connect(ClientContext *ctx);

// 한 줄씩 읽고 보내는 헬퍼
int read_line(ClientContext *ctx, int sock, char *buf, size_t size);
int send_line(int sock, const char *line);

#endif