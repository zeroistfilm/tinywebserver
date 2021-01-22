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
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd); // 트랜잭션 수행
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
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE); // 먼저 요청 라인을 읽고
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version); // 분석한다.
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);
    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &sbuf) < 0) {
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }
    if (is_static) { /* Serve static content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size);
    }
    else { /* Serve dynamic content */
        if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
            clienterror(fd, filename, "403", "Forbidden",
                        "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}
void clienterror(int fd, char* cause, char* errnum,
                 char* shortmsg, char* longmsg)
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
void read_requesthdrs(rio_t* rp)
{
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}


void serve_dynamic(int fd, char *filename, char *cgiargs) {
    char buf[MAXLINE], *emprylist[] = {NULL};
    sprintf(buf, "HTTP/1.0 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server : Tiny web server \r \n");
    Rio_writen(fd, buf, strlen(buf));
    if (Fork() == 0) {
        // 환경변수 추가 환경변수 이름, 변수 값, 덮어 쓸거니?
        setenv("Query_string", cgiargs, 1);
        Dup2(fd, STDOUT_FILENO);
        Execve(filename,emprylist,environ);
    }
    Wait(NULL);
}

void serve_static(int fd, char *filename, int filesize){
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    /* send response headers to client*/
    get_filetype(filename, filetype);  // 접미어를 통해 파일 타입 결정한다.
    // 클라이언트에게 응답 줄과 응답 헤더 보낸다
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer : Tiny Web Server \r\n", buf);
    sprintf(buf, "%sConnection : close \r\n", buf);
    sprintf(buf, "%sConnect-length : %d \r\n", buf, filesize);
    sprintf(buf, "%sContent-type : %s \r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers : \n");
    printf("%s", buf);
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    // mmap() 으로 만들어진 맵핑을 제거하기 위한 시스템 호출이다
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

    if (!strstr(uri, "cgi-bin")) { /* Static content*/
        strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if (uri[strlen(uri)-1] == '/')
            strcat(filename, "home.html");
        return 1;
    }
    else { /* Dynamic content*/
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgiargs, ptr+1);
            *ptr = '\0';
        }
        else
            strcpy(cgiargs, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;
    }
}