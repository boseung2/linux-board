# ===========================
# 리눅스 게시판 프로젝트 Makefile
# ===========================

# 컴파일러
CC = gcc
CFLAGS = -Wall

# ---------------------------
# 서버 설정
# ---------------------------

SERVER_DIR = server
SERVER_SRC = $(SERVER_DIR)/server.c $(SERVER_DIR)/user.c
SERVER_OUT = $(SERVER_DIR)/server

server:
	$(CC) $(CFLAGS) -o $(SERVER_OUT) $(SERVER_SRC)

# ---------------------------
# 클라이언트 설정
# ---------------------------

CLIENT_DIR = client
CLIENT_SRC = \
	$(CLIENT_DIR)/main.c \
	$(CLIENT_DIR)/ui_main_menu.c \
	$(CLIENT_DIR)/ui_auth.c \
	$(CLIENT_DIR)/ui_board.c \
	$(CLIENT_DIR)/socket.c

CLIENT_OUT = $(CLIENT_DIR)/client

client:
	$(CC) $(CFLAGS) -o $(CLIENT_OUT) $(CLIENT_SRC)

# ---------------------------
# 전체 빌드
# ---------------------------
all: server client


# ---------------------------
# 클린업
# ---------------------------

clean:
	rm -f $(SERVER_OUT) $(CLIENT_OUT)

.PHONY: all server client clean