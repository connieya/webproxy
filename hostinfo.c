#include "csapp.h"

int main(int argc, char **argv)
{
    struct addrinfo *p , *listp , hints;
    char buf[MAXLINE];
    int rc , flags;

    if(argc != 2) {
        fprintf(stderr , "usage : %s <domain name> \n", argv[0]);
    }
    return 0;
}
