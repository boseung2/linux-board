# 리눅스 게시판 프로젝트

@boseung2  
@lia0519

## 프로젝트 실행 요약

```
make clean
make
```

```
./server/server
```

```
./client/client
```

## 전체 빌드

서버와 클라이언트를 한 번에 빌드합니다.

```
make clean
make
```

성공적으로 빌드되면 다음 실행 파일이 생성됩니다:

```
server/server
client/client
```

---

## 서버만 빌드

```
make server
```

아래 실행 파일이 생성됩니다:

```
server/server
```

---

## 클라이언트만 빌드

```
make client
```

아래 실행 파일이 생성됩니다:

```
client/client
```

---

## 서버 실행

```
./server/server

Server is running on port 9000
```

---

## 클라이언트 실행

```
./client/client

Connected to server 127.0.0.1:9000
```

## 실행 파일 삭제

```
make clean
```
