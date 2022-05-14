/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

void get_filetype(char *filename , char *filetype)
{
  // strstr 함수 : 문자열 안에서 문자열을 검색하는 함수 , 문자열 중에 특정 내용이 포함되어 있는지 조사하는 함수
  if(strstr(filename , ".html")) {
    strcpy(filetype , "text/html");
  }
  else if(strstr(filename , ".gif")){
    strcpy(filetype , "image/gif");
  }
  else if (strstr(filename, ".png"))
  {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg"))
  {
    strcpy(filetype, "image/jpeg");
  }
  else {
    strcpy(filetype , "text/plain");
  }
}


void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp , buf , MAXLINE);
  while (strcmp(buf , "\r\n")) // strcmp : 두 문자열을 비교하는 함수
  {                            // \r : carriage return  : 커서를 그 줄 맨 앞으로 리턴하는 제어 문자
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
  
}

void serve_dynamic(int fd , char * filename , char *cgiargs)
{
  // MAXLINE = MAX TEXT LINE Length : 8192
  char buf[MAXLINE], *emptylist [] = { NULL };

  sprintf(buf , "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd , buf , strlen(buf));
  sprintf(buf , "Server: Tiny Web Server \r\n");
  Rio_writen(fd , buf , strlen(buf));

  if(Fork() == 0) { // 새로운 자식 프로세스를 fork 한다.
    setenv("QUERY_STRING" , cgiargs , 1);

    // STDOUT_FILENO :  linux 시스템의 파일 디스크립터로 type 이 int 이고 unist.h에 정으 되어 있다.
    // STDIN_FILENO - 표준입력(식별자 0) , STDOUT_FILENO - 표준 출력(식별자 1) , STDERR-FILENO(식별자 2)
    Dup2(fd , STDOUT_FILENO); // 자식은 자식의 표준 출력을 연결 파일 식별자로 재지정한다.
    Execve(filename , emptylist , environ);
  }
  Wait(NULL); // 부모는 자식이 종료되어 정리되는 것을 기다리기 위해서 wait 함수에서 블록된다.
}

void serve_static(int fd , char *filename , int filesize)
{
  int srcfd;
  char *srcp , filetype[MAXLINE] , buf[MAXBUF];

  // send response headers to client
  get_filetype(filename , filetype); // 파일 이릉의 접미어 부분을 검사해서 파일 타입을 결정한다.
  sprintf(buf , "HTTP/1.0 200 OK\r\n"); // sprintf 함수 : 문자열을 buf 배열로 보낸다.
  sprintf(buf , "%sServer : Tiny Web Server \r\n" , buf);
  sprintf(buf , "%sConnection : close \r\n" , buf);
  sprintf(buf , "%sContent-length: %d\r\n",  buf , filesize);
  sprintf(buf , "%sContent-type : %s\r\n\r\n" , buf, filetype);
  Rio_writen(fd , buf , strlen(buf));
  printf("Response headers: \n");
  // 클라이언트에게 응답을 보낸다.

  printf("%s" , buf);

  // send response body to client
  // 리턴 값 식별자 번호를 저장
  srcfd = Open(filename , O_RDONLY , 0); // filename 을 오픈하고 식별자를 얻어온다. , Reading only
  srcp = Mmap(0, filesize , PROT_READ , MAP_PRIVATE , srcfd , 0); // mmap 함수는 요청한 파일을 가상 메모리 영역으로 매핑한다.
  Close(srcfd); // 파일을 메모리로 매핑한 후에 파일을 닫는다. 
  Rio_writen(fd , srcp , filesize);
  Munmap(srcp , filesize); // 매핑된 가상 메모리 주소를 반환한다.
}


int parse_uri(char * uri , char * filename , char *cgiargs) {
  char * ptr;

  // strstr 함수 : 문자열 안에서 문자열을 검색하는 함수 , 문자열 중에 특정 내용이 포함되어 있는지 조사하는 함수
  // 문자열을 발견하면 그 문자열의 시작 번지를 리턴한다. 없으면 NULL 을 리턴
  if(!strstr(uri , "cgi-bin")) { // 정적 콘텐츠 라면

    // 정적 콘텐츠라면 CGI 인자 스트링을 지운다.
    strcpy(cgiargs , ""); // strcpy 함수 : 문자열을 복사하는 함수 

    // uri 을 ./index.html 같은 상대 리눅스 경로 이름으로 변환한다.
    strcpy(filename , ".");
    strcat(filename , uri); // strcat 함수 : 문자열을 연결하는 함수이다. 첫번째 인자 문자열 뒤에 두번째 인자 문자열을 덧 붙인다.

  
    if(uri[strlen(uri)-1] == '/'){ // uri 가  '/' 문자로 끝난다면
      strcat(filename , "home.html");  // 기본 파일 이름을 추가한다.
    } 
    return 1;
  }
  else { // 동적 콘텐츠 라면

      // 모든 CGI 인자들을 추출한다.
      ptr = index(uri , '?');
      if(ptr) {
        strcpy(cgiargs , ptr+1);
        *ptr = '\0';
      }else {
        strcpy(cgiargs , "");
      }

      // 나머지 uri 부분을 상대 리눅스 파일 이름으로 변환한다.
      strcpy(filename , ".");
      strcat(filename , uri);
      return 0;
  }

}

void clienterror(int fd , char *cause , char *errnum  ,  char *shortmsg , char * longmsg) {

  char buf[MAXLINE] , body[MAXBUF];

  // Build the HTTP response body
  // int sprintf(char *buffer , const char *format , ...); : 서식화된 문자열을 첫 번쩨 인수로 전달된 buffer 배열로 보낸다.
  sprintf(body, "<html><title>Tiny Error</title>"); // sprintf : 배열 버퍼에 일련의 문자와 값의 형식을 지정하고 저장한다.
  sprintf(body , "%s<body bgcolor = ""fffffff"">\r\n" ,  body); // sprintf : 서식화된 문자열을 조립한 후
  sprintf(body , "%s%s: %s\r\n", body , errnum , shortmsg); // 화면으로 그대로 출력할 수 도 있고 파일로도 출력할 수 있으며 그래픽 화면으로도 출력할 수 있다.
  sprintf(body, "%s<p>%s : %s\r\n" , body , longmsg , cause);
  sprintf(body, "%s<hr><em>The Tiny Web Server </em>\r\n", body);
  // 상태 코드와 상태메세지를 포함하며 HTTP 응답을 보낸다.

  // print the HTTP response
  sprintf(buf , "HTTP/1.0 %s %s \r\n" , errnum , shortmsg);

  // 버퍼 없는 출력 함수  , buf에서 식별자 fd로 strlen(buf) 바이트 전송한다.
  Rio_writen(fd , buf , strlen(buf));
  sprintf(buf , "Content-type : text/html\r\n");
  Rio_writen(fd , buf , strlen(buf));
  sprintf(buf , "Content-length : %d\r\n\r\r" , (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));

}


void doit(int fd) // HTTP 트랜 잭션을 처리한다.
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE] , method[MAXLINE] , uri[MAXLINE] , version[MAXLINE];
  char filename[MAXLINE] , cgiargs[MAXLINE];
  rio_t rio;


  // 요청 라인을 읽고 분석한다.
  // Read request line and headers
  Rio_readinitb(&rio , fd); // 식별자 fd를 주소 rio에 위치한 rio_t 타입의 읽기 버퍼와 연결한다.

  Rio_readlineb(&rio , buf , MAXLINE); // 텍스트 라인 전체를 내부 읽기 버퍼에서 복사하는 함수 ,
  // 버퍼가 비워지게 될 때마다 버퍼를 다시 채우기 위해 자동으로 read를 호출한다.

  printf("Request headers : \n");
  printf("%s",buf);
  sscanf(buf, "%s %s %s" , method , uri , version); // sscanf 함수 : 문자열에서 형식화 된 데이터를 읽어온다.
  // method , uri , version 을 buf 문자열 로 읽어 온다. 

  if(strcasecmp(method , "GET")) { // strcasecmp 함수 : 대소문자를 구분하지 않고 문자열을 비교 한다.
    // tiny 는 get method 만 지원한다.  다른 메소드를 요청하면 에러메시지를 보내고 main 함수로 돌아간다.
    clienterror(fd , method , "501" , "Not implemented" , "Tiny does not implement this method");

    return;
  }
  read_requesthdrs(&rio);

  is_static = parse_uri(uri , filename , cgiargs); // 동적 or 정적 콘텐츠 인지 확인 

  if(stat(filename , &sbuf) < 0) { // stat 함수를 이용하면 파일의 상태를 알아올 수 있다. 
  // stat 첫번째 인자 filename 의 상태를 얻어와서 두번째 인자인 stat 구조체  sbuf에 채워 넣는다. 
  // 성공할 경우 0 , 실패 할 경우 -1을 리턴한다.

    clienterror(fd , filename , "404" , "Not found" , "Tiny couldn't find this file");

    return;
  }

  if(is_static) { // 정적 컨텐츠 라면

    //    S_ISREG – 정규 파일인지 판별,  S_IRUSR : user can read this file
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { // 읽기 권한이 있는지
      clienterror(fd , filename , "403" , "Forbidden" , "Tiny couldn't read the file");

      return;
    }
    serve_static(fd , filename , sbuf.st_size); // 정적 콘텐츠를 클라이언트에게 제공한다.
  }
  else
  { // 동적 콘텐츠 라면
    //     S_ISREG – 정규 파일인지 판별 ,  S_IXUSR : user can execute this file
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode))
    { // 읽기 권한이 있는지
      clienterror(fd , filename , "403" , "Forbidden" , "Tiny couldn't run the CGI program");

      return;
    } 
    serve_dynamic(fd , filename , cgiargs); // 동적 컨텐츠를 제공한다.
  }
}

// 소켓은 네트워크 상의 다른 프로세스와 통신하기 위해 사용되는 파일
int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // Open_listenfd 함수를 호출해서 listen socket을 오픈한다.
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}