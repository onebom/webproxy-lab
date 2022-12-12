/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE],content[MAXLINE];
  int n1=0,n2=0;

  if((buf=getenv("QUERY_STRING"))!=NULL){
    p=strchr(buf,'&');
    *p='\0';  //이거 무슨뜻임? 문자열로생각해서 NULL을 넣은건가?...
    strcpy(arg1,buf);
    strcpy(arg2,p+1);
    n1 = atoi(arg1);
    n2 = atoi(arg2);
  }

  sprintf(content,"QUERY_STRING=%s",buf);
  sprintf(content,"welcome to add.com:");
  sprintf(content,"%sTHE Internet addition...\r\n<p>",content);
  sprintf(content,"%sThe answer is: %d + %d = %d\r\n<p>",content,n1,n2,n1+n2);
  sprintf(content,"%sThanks for visiting!\r\n",content);
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n",(int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s",content);
  fflush(stdout);

  exit(0);
}
/* $end adder */
