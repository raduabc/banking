#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define BUFLEN 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, n, i, fdmax, conectat = 0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	fd_set readfds;
	fd_set tmpfds;

    char buffer[BUFLEN];
    if (argc < 3) {
       fprintf(stderr,"Usage %s server_ip server_port\n", argv[0]);
       exit(0);
    }  
    
	FD_ZERO(&readfds);
	FD_ZERO(&tmpfds);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    FD_SET(0, &readfds);
	FD_SET(sockfd, &readfds);
	fdmax = sockfd;
    
    sprintf(buffer, "client-%d.log", getpid());
	FILE *f = fopen(buffer, "w");

    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");    
    
    while(1){
  		//citesc de la tastatura
		tmpfds = readfds;
		select(fdmax + 1, &tmpfds, NULL, NULL, NULL);
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmpfds)) {
				if (i == 0) {
					memset(buffer, 0 , BUFLEN);
    				fgets(buffer, BUFLEN-1, stdin);
    				fprintf(f, "%s", buffer);
    				if(!strstr(buffer, "login") || conectat == 0) {
						send(sockfd, buffer, strlen(buffer), 0);
						if(strstr(buffer, "quit")) {
							close(sockfd);
							return 0;
						}
					} else {
						fprintf(f, "IBANK> -2: Sesiune deja deschisa");
						printf("IBANK> -2: Sesiune deja deschisa");
					}
				} else if (i == sockfd) {
					memset(buffer, 0 , BUFLEN);
					recv(i, buffer, sizeof(buffer), 0);
					if(strstr(buffer, "Welcome"))
						conectat = 1;
					if(strstr(buffer, "deconectat"))
						conectat = 0;
					printf("%s\n", buffer);
					fprintf(f, "%s\n", buffer);
				}
			}		
		}
    }
    close(sockfd);
    return 0;
}


