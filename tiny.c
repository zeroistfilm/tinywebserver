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
        doit(connfd); // 트랜잭션 수행   kjkjlkjsdasd
        Close(connfd); // 자신 쪽의 연결 끝을 닫는다.
        printf("===============================================\n\n");

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
    Rio_readinitb(&rio, fd); // &rio 주소를 가지는 버퍼를 만든다.
    Rio_readlineb(&rio, buf, MAXLINE); // 버퍼에서 읽은 것이 담겨있다.
    printf("Request headers:\n");
    printf("%s", buf); // "GET / HTTP/1.1"
    sscanf(buf, "%s %s %s", method, uri, version); // 버퍼에서 자료형을 읽는다, 분석한다.

    //메소드가 get이 아니면 에러를 뱉고 종료한다
    if (strcasecmp(method, "GET")) { //문자열의 대소를 무시하 비교한다.
        clienterror(fd, method, "501", "Not implemented",
                    "Tiny does not implement this method");
        return;
    }
    // get인경우 다른 요청 헤더를 무시한다.

    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    //uri를 분석한다.
    // 파일이 없는 경우 에러를 띄운다/
    //parse_uri를 들어가기 전에 filename과 cgiargs는 없다.
    is_static = parse_uri(uri, filename, cgiargs);
    printf("uri : %s, filename : %s, cgiargs : %s \n", uri, filename, cgiargs);

    if (stat(filename, &sbuf) < 0) { //stat는 파일 정보를 불러오고 sbuf에 내용을 적어준다. ok 0, errer -1
        clienterror(fd, filename, "404", "Not found",
                    "Tiny couldn't find this file");
        return;
    }

    // 정적 콘텐츠일경우
    if (is_static) { /* Serve static content */
        //파일 읽기 권한이 있는지 확인하기
        //S_ISREG : 일반 파일인가? , S_IRUSR: 읽기 권한이 있는지? S_IXUSR 실행권한이 있는가?
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
        //http://52.79.55.247:1024/cgi-bin/adder?123&456
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

//클라이언트에게 오류 보고 한다
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
    while (strcmp(buf, "\r\n")) {  //끝줄 나올 때 까지 읽는다
        Rio_readlineb(rp, buf, MAXLINE);
    }
    return;
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response*/
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf)); //왜 두번 쓰나요?
    sprintf(buf,"Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    //fd는 정보를 받자마자 전송하나요???
    //클라이언트는 성공을 알려주는 응답라인을 보내는 것으로 시작한다.
    if (Fork() == 0) { //타이니는 자식프로세스를 포크하고 동적 컨텐츠를 제공한다.
        setenv("QUERY_STRING", cgiargs, 1);
        //자식은 QUERY_STRING 환경변수를 요청 uri의 cgi인자로 초기화 한다.  (15000 & 213)
        Dup2(fd, STDOUT_FILENO); //자식은 자식의 표준 출력을 연결 파일 식별자로 재지정하고,

        Execve(filename, emptylist, environ);
        // 그 후에 cgi프로그램을 로드하고 실행한다.
        // 자식은 본인 실행 파일을 호출하기 전 존재하던 파일과, 환경변수들에도 접근할 수 있다.

    }
    Wait(NULL); //부모는 자식이 종료되어 정리되는 것을 기다린다.
}

// fd 응답받는 소켓(연결식별자), 파일 이름, 파일 사이즈
void serve_static(int fd, char *filename, int filesize){
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
    /* send response headers to client*/
    //파일 접미어 검사해서 파일이름에서 타입 가지고
    get_filetype(filename, filetype);  // 접미어를 통해 파일 타입 결정한다.
    // 클라이언트에게 응답 줄과 응답 헤더 보낸다기
    // 클라이언트에게 응답 보내기
    // 데이터를 클라이언트로 보내기 전에 버퍼로 임시로 가지고 있는다.
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer : Tiny Web Server \r\n", buf);
    sprintf(buf, "%sConnection : close \r\n", buf);
    sprintf(buf, "%sConnect-length : %d \r\n", buf, filesize);
    sprintf(buf, "%sContent-type : %s \r\n\r\n", buf, filetype);
    //rio_readn은 fd의 현재 파일 위치에서 메모리 위치 usrbuf로 최대 n바이트를 전송한다.
    //rio_writen은 usrfd에서 식별자 fd로 n바이트를 전송한다.
    //서버에 출력
    printf("Response headers : \n");
    printf("%s", buf);

    //읽을 수 있는 파일로 열기
    srcfd = Open(filename, O_RDONLY, 0); //open read only 읽고
    //PROT_READ -> 페이지는 읽을 수만 있다.
    // 파일을 어떤 메모리 공간에 대응시키고 첫주소를 리턴
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); //메모리로 넘기고
    //매핑위치, 매핑시킬 파일의 길이, 메모리 보호정책, 파일공유정책,srcfd ,매핑할때 메모리 오프셋
//    srcp = malloc(sizeof(srcfd));

    Close(srcfd);//닫기


    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, srcp, filesize);
    // mmap() 으로 만들어진 -맵핑을 제거하기 위한 시스템 호출이다
    // 대응시킨 녀석을 풀어준다. 유효하지 않은 메모리로 만듦
    // void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
    // int munmap(void *start, size_t length);
    Munmap(srcp, filesize); //메모리 해제
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
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");

}

