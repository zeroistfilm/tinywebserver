/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content
 */
#include "csapp.h"
void doit(int fd);
void read_requesthdrs(rio_t* rp);
int parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize);
void get_filetype(char* filename, char* filetype);
void serve_dynamic(int fd, char* filenmae, char* cgiargs);
void clienterror(int fd, char* cause, char* errnum,
                 char* shortmsg, char* longmsg);
/**
 *	@fn		main
 *
 *	@brief                      Tiny main 루틴, TINY는 반복실행 서버로 명령줄에서 넘겨받은
 *                              포트로의 연결 요청을 듣는다. open_listenfd 함수를 호출해서
 *                              듣기 소켓을 오픈한 후에, TINY는 전형적인 무한 서버 루프를
 *                              실행하고, 반복적으로 연결 요청을 접수하고, 트랙잭션을
 *                              수행하고, 자신 쪽의 연결 끝을 닫는다.
 *
 *  @param  int argc
 *  @param  char **argv
 *
 *	@return	int
 */
// fd : 파일 또는 소켓을 지칭하기 위해 부여한 숫자
// socket
int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    /* Check command-line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    listenfd = Open_listenfd(argv[1]);
    while (1) { // 무한 서버 루프
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen); // 반복적으로 연결 요청을 접수
        Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        printf("Accepted connection from (%d ,%s, %s)\n",connfd, hostname, port);
        doit(connfd); // 트랜잭션 수행   kjkjlkjsdasd
        Close(connfd); // 자신 쪽의 연결 끝을 닫는다.
    }
}
/**
 *	@fn		doit
 *
 *	@brief                      Tiny main 루틴, TINY는 반복실행 서버로 명령줄에서 넘겨받은
 *                              포트로의 연결 요청을 듣는다. open_listenfd 함수를 호출해서
 *                              듣기 소켓을 오픈한 후에, TINY는 전형적인 무한 서버 루프를
 *                              실행하고, 반복적으로 연결 요청을 접수하고, 트랙잭션을
 *                              수행하고, 자신 쪽의 연결 끝을 닫는다.
 *
 *  @param  int argc
 *  @param  char **argv
 *
 *	@return	void
 */
void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;
    /* Read request line and headers */
    Rio_readinitb(&rio, fd); // rio 구조체 초기화
    Rio_readlineb(&rio, buf, MAXLINE); // 버퍼에서 읽은 것이 담겨있다.
    printf("Request headers:\n");
    printf("%s", buf); // "GET / HTTP/1.1"
    sscanf(buf, "%s %s %s", method, uri, version); // 분석한다.

    //메소드가 get이 아니면 에러를 뱉고 종료한다
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny does not implement this method");
        return;
    }
    // get인경우 다른 요청 헤더를 무시한다.
    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    //uri를 분석한다.
    // 파일이 없는 경우 에러를 띄운다/
    is_static = parse_uri(uri, filename, cgiargs);
    printf(" ========= uri : %s, filename : %s, cgiargs : %s ========= \n", uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    // 정적 콘텐츠일경우
    if (is_static) { /* Serve static content */
        //파일 읽기 권한이 있는지 확인하기
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            //권한이 없다면 클라이언트에게 에러를 전달
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        //그렇다면 클라이언트에게 파일 제공
        serve_static(fd, filename, sbuf.st_size);
    }//정적 컨텐츠가 아닐경우
   else { /* Serve dynamic content */
        // 파일이 실행가능한 것인지
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            //실행이 불가능하다면 에러를 전
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        //그렇다면 클라이언트에게 파일 제공.
        serve_dynamic(fd, filename, cgiargs);
    }
}
//클라이언트에게 오류 보고 하
void clienterror(int fd, char* cause, char* errnum,char* shortmsg, char* longmsg)
{
    char buf[MAXLINE], body[MAXBUF];
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

//요청 헤더 읽기
void read_requesthdrs(rio_t* rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE); // 버터에서 MAXLINE 까지 읽기
    while (strcmp(buf, "\r\n")) {  //끝줄 나올 때 까지 읽
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response*/
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf,"Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename, emptylist, environ);
    }
    Wait(NULL);
}

// fd 응답받는 소켓(연결식별자), 파일 이름, 파일 사이즈
void serve_static(int fd, char *filename, int filesize){
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    /* send response headers to client*/
    //파일 접미어 검사해서 파일이름에서 타입 가지고 오
    get_filetype(filename, filetype);  // 접미어를 통해 파일 타입 결정한다.
    // 클라이언트에게 응답 줄과 응답 헤더 보낸다기
    // 클라이언트에게 응답 보내기
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer : Tiny Web Server \r\n", buf);
    sprintf(buf, "%sConnection : close \r\n", buf);
    sprintf(buf, "%sConnect-length : %d \r\n", buf, filesize);
    sprintf(buf, "%sContent-type : %s \r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));

    //서버에 출력
    printf("Response headers : \n");
    printf("%s", buf);

    //읽을 수 있는 파일로 열기
    srcfd = Open(filename, O_RDONLY, 0);
    //PROT_READ -> 페이지는 읽을 수만 있다.
    // 파일을 어떤 메모리 공간에 대응시키고 첫주소를 리턴
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    // mmap() 으로 만들어진 맵핑을 제거하기 위한 시스템 호출이다
    // 대응시킨 녀석을 풀어준다. 유효하지 않은 메모리로 만듦
    // void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
    // int munmap(void *start, size_t length);
    Munmap(srcp, filesize);
}

void get_filetype(char *filename, char *filetype){
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filename, "image/.png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/placin");
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *ptr;
    //cgi-bin이 없다면
    if (!strstr(uri, "cgi-bin")) { /* Static content*/
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri); // ./uri/home.html 이 된다
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else { /* Dynamic content*/
        ptr = index(uri, '?');
        //CGI인자 추출
        if (ptr) {
            // 물음표 뒤에 있는 인자 다 갖다 붙인다.
            strcpy(cgiargs, ptr+1);
            //포인터는 문자열 마지막으로 바꾼다.
            *ptr = '\0'; // uri물음표 뒤 다 없애기
        }
        else
            strcpy(cgiargs, ""); // 물음표 뒤 인자들 전부 넣기
        strcpy(filename, "."); //나머지 부분 상대  uri로 바꿈,
        strcat(filename, uri); // ./uri 가 된다.
        return 0;
    }
}