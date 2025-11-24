#ifndef AUTH_H
#define AUTH_H

int handle_signup(int fd, const char *id, const char *pw);
int handle_login(int fd, const char *id, const char *pw);
int handle_quit(int fd);

#endif