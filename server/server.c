#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/select.h>

#include "user.h"

#define PORT 9000
#define MAX_CLIENTS FD_SETSIZE
#define BUF_SIZE 1024

void handle_command(int fd, char *line) {
    char cmd[16], id[32], pw[32];

    // 개행 제거
    line[strcspn(line, "\r\n")] = '\0';
    printf("[DEBUG] from fd=%d: %s\n", fd, line);

    if (sscanf(line, "%15s %31s %31s", cmd, id, pw) >= 1) {

        // SIGNUP
        if (strcmp(cmd, "SIGNUP") == 0) {
            int res = user_signup(id, pw);
            if (res == USER_OK) {
                const char *msg = "OK SIGNUP\n";
                write(fd, msg, strlen(msg));
            } else if (res == USER_ERR_EXISTS) {
                const char *msg = "FAIL SIGNUP ID_EXISTS\n";
                write(fd, msg, strlen(msg));
            } else if (res == USER_ERR_FULL) {
                const char *msg = "FAIL SIGNUP SERVER_FULL\n";
                write(fd, msg, strlen(msg));
            } else {
                const char *msg = "FAIL SIGNUP IO_ERROR\n";
                write(fd, msg, strlen(msg));
            }
            return;
        }

        // LOGIN
        else if (strcmp(cmd, "LOGIN") == 0) {
            int res = user_login(id, pw);
            if (res == USER_OK) {
                const char *msg = "OK LOGIN\n";
                write(fd, msg, strlen(msg));
            } else if (res == USER_ERR_NO_SUCH_ID) {
                const char *msg = "FAIL LOGIN NO_SUCH_ID\n";
                write(fd, msg, strlen(msg));
            } else if (res == USER_ERR_WRONG_PW) {
                const char *msg = "FAIL LOGIN WRONG_PASSWORD\n";
                write(fd, msg, strlen(msg));
            } else {
                const char *msg = "FAIL LOGIN UNKNOWN_ERROR\n";
                write(fd, msg, strlen(msg));
            }
            return;
        }

        // QUIT
        else if (strcmp(cmd, "QUIT") == 0) {
            const char *msg = "BYE\n";
            write(fd, msg, strlen(msg));
            // 여기서는 연결은 유지하고, 클라이언트가 종료할지 말지는 나중에 설계해도 됨
            return;
        }
    }

    const char *msg = "FAIL UNKNOWN_COMMAND\n";
    write(fd, msg, strlen(msg));
}

int main() {
  int serv_sock, clnt_sock;
  struct sockaddr_in serv_addr, clnt_addr;
  socklen_t clnt_addr_len;

  fd_set reads, cpy_reads;
  int fd_max;

  char buf[BUF_SIZE];

  // 🔹 유저 시스템 초기화 (파일에서 불러오기)
  user_system_init();

  // 1. 서버 소켓 생성
  serv_sock = socket(AF_INET, SOCK_STREAM, 0);
  if(serv_sock == -1) {
    perror("socket() error");
    exit(1);
  }

   // SO_REUSEADDR 옵션 (서버 재시작시 TIME_WAIT 문제 방지)
  int opt = 1;
  setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // 2. bind
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(PORT);

  if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
    perror("bind() error");
    close(serv_sock);
    exit(1);
  }

  // 3. listen
  if(listen(serv_sock, 5) == -1) {
    perror("listen() error");
    close(serv_sock);
    exit(1);
  }

  printf("Server is running on port %d\n", PORT);

  FD_ZERO(&reads);
  FD_SET(serv_sock, &reads);
  fd_max = serv_sock;

  while(1) {
    cpy_reads = reads;
    int num_ready = select(fd_max + 1, &cpy_reads, NULL, NULL, NULL);
    if(num_ready == -1) {
      perror("select() error");
      break;
    }

    for(int fd=0; fd<=fd_max; fd++) {
      if(!FD_ISSET(fd, &cpy_reads)) continue;

      if(fd==serv_sock) {
        clnt_addr_len = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_len);
        if (clnt_sock == -1) {
          perror("accept");
          continue;
        }

        FD_SET(clnt_sock, &reads);
        if (clnt_sock > fd_max) fd_max = clnt_sock;

        printf("New client connected: fd=%d\n", clnt_sock);
        const char *msg = "Welcome! Use SIGNUP/LOGIN/QUIT\n";
        write(clnt_sock, msg, strlen(msg));
      }

      else {
        int len = read(fd, buf, BUF_SIZE - 1);
        if (len <= 0) {
            // 연결 종료
            if (len == 0) {
                printf("Client disconnected: fd=%d\n", fd);
            } else {
                perror("read");
            }
            close(fd);
            FD_CLR(fd, &reads);
        } else {
            buf[len] = '\0';
            // 한 번에 여러 줄이 올 수도 있지만,
            // 예시는 "한 줄씩" 온다고 가정
            handle_command(fd, buf);
        }
      }
    }
  }

  close(serv_sock);
  return 0;
}
