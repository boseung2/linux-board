# 리눅스 게시판 프로젝트

@boseung2  
@lia0519

## 서버 빌드

```
cd linux-board/server

gcc -o server server.c user.c
```

---

## 클라이언트 빌드

```
cd linux-board/client

gcc -o client client.c
```

---

## 서버 실행

```
cd linux-board/server
./server

Server is running on port 9000
```

---

## 클라이언트 실행

```
Server is running on port 9000

Connected to server 127.0.0.1:9000
Commands: SIGNUP <id> <pw>, LOGIN <id> <pw>, QUIT
[SERVER] Welcome! Use SIGNUP/LOGIN/QUIT
```
