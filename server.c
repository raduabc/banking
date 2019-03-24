#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS	8
#define BUFLEN 1024

typedef struct card_data {
	char nume[20], prenume[20], numar[7], pin[5], parola[10];
	double sold;
	int wrong, blocat;
} card_data;


void error(char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	int n, i, j, l, ret;
	double dbl;
	char buffer[BUFLEN];

	if (argc < 3) {
		fprintf(stderr,"Usage : %s port data_file\n", argv[0]);
		exit(1);
	}

	FILE *f;
	f = fopen (argv[2], "r");
	fscanf(f, "%s", buffer);
	n = atoi(buffer);
	card_data *data_vec = malloc ((n + 1) * sizeof(card_data));

	for (i = 1; i <= n; i++) {
		memset(buffer, 0, BUFLEN);
		fscanf(f, "%s",  buffer);
		memcpy(data_vec[i].nume, buffer, strlen(buffer));
		//printf("%s ", buffer);

		memset(buffer, 0, BUFLEN);
		fscanf(f, "%s",  buffer);
		memcpy(data_vec[i].prenume, buffer, strlen(buffer));
		//printf("%s ", buffer);

		memset(buffer, 0, BUFLEN);
		fscanf(f, "%s",  buffer);
		memcpy(data_vec[i].numar, buffer, strlen(buffer));
		//printf("%s ", buffer);

		memset(buffer, 0, BUFLEN);
		fscanf(f, "%s",  buffer);
		memcpy(data_vec[i].pin, buffer, strlen(buffer));
		//printf("%s ", buffer);

		memset(buffer, 0, BUFLEN);
		fscanf(f, "%s",  buffer);
		memcpy(data_vec[i].parola, buffer, strlen(buffer));
		//printf("%s ", buffer);

		fscanf(f, "%lf", &dbl);
		data_vec[i].sold = dbl;
		data_vec[i].wrong = 0;
		data_vec[i].blocat = 0;
		//printf("%.2lf\n", dbl);
	}
	fclose(f);


	int socktcp, newsocktcp, sizeof_sockaddr;
	struct sockaddr_in serv_addr, cli_addr;

	fd_set read_fds;  // multimea de citire folosita in select()
	fd_set tmp_fds;
	int fdmax;  // valoare maxima file descriptor din multimea read_fds

	//golim multimea de descriptori de citire (read_fds) si multimea tmp_fds 
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);	

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
	serv_addr.sin_port = htons(atoi(argv[1]));
	
	socktcp = socket(AF_INET, SOCK_STREAM, 0);
	if (socktcp < 0) 
	   error("ERROR opening socket");

	if (bind(socktcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) < 0) 
		error("ERROR on binding");
	
	listen(socktcp, MAX_CLIENTS);

	//adaugam noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(socktcp, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = socktcp;

	int client_conectat = 0;
	char *delim = " \n,", *token;

	while (1) {
		tmp_fds = read_fds; 
		if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1) 
			error("ERROR in select");
	
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
			
				if (i == socktcp) {
					// a venit ceva pe socketul inactiv(cel cu listen) = o noua conexiune
					// actiunea serverului: accept()
					sizeof_sockaddr = sizeof(cli_addr);
					if ((newsocktcp = accept(socktcp, (struct sockaddr *)&cli_addr, &sizeof_sockaddr)) == -1) {
						error("ERROR in accept");
					}
					else {
						//adaug noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(newsocktcp, &read_fds);
						if (newsocktcp > fdmax) { 
							fdmax = newsocktcp;
						}
					}
					printf("Noua conexiune de la %s, port %d, socket_client %d\n ", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsocktcp);
					continue;
				}
				
				//citire de la tastatura
				if (i == 0) {
					memset(buffer, 0 , BUFLEN);
    				fgets(buffer, BUFLEN-1, stdin);
    				if (strstr(buffer, "quit")) {
    					close(socktcp);
    					return 0;
    				}
				}

				// am primit date pe unul din socketii cu care vorbesc cu clientii
				//actiunea serverului: recv()
				memset(buffer, 0, BUFLEN);
				if ((ret = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
					if (ret == 0) {
						//conexiunea s-a inchis
						printf("selectserver: socket %d hung up\n", i);
					} else {
						error("ERROR in recv");
					}
					client_conectat = 0;
					close(i);
					FD_CLR(i, &read_fds); // scoatem din multimea de citire socketul
				}
				
				else { //recv intoarce > 0
					printf ("Am primit de la clientul de pe socketul %d, mesajul: %s\n", i, buffer);

					token = strtok(buffer, delim);

					if (strcmp(token, "login") == 0) {
						if (client_conectat) {
							// sesiune deja activa
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "IBANK> -2: Sesiune deja deschisa");
							send(i, buffer, strlen(buffer), 0);
						} else {
							token = strtok(NULL, delim);
							for (j = 1; j <= n; j++) {
								if (strcmp(token, data_vec[j].numar) == 0) { // numar corect
									token = strtok(NULL, delim);
									if (strcmp(token, data_vec[j].pin) == 0) { // pin corect
										if (data_vec[j].blocat) { // cardul este deja blocat
											memset(buffer, 0, BUFLEN);
											sprintf(buffer, "IBANK> -5: Card blocat");
											send(i, buffer, strlen(buffer), 0);
										} else { // logare reusita
											client_conectat = j;
											data_vec[j].wrong = 0;
											memset(buffer, 0, BUFLEN);
											sprintf(buffer, "IBANK> Welcome %s %s", data_vec[j].nume, data_vec[j].prenume);
											send(i, buffer, strlen(buffer), 0);
										}
									} else { // pin gresit
										data_vec[j].wrong ++;
										// verificare daca este/trebuie blocat
										if (data_vec[j].wrong >= 3) {
											data_vec[j].blocat = 1;
											memset(buffer, 0, BUFLEN);
											sprintf(buffer, "IBANK> -5: Card blocat");
											send(i, buffer, strlen(buffer), 0);
										} else { // pin gresit
											memset(buffer, 0, BUFLEN);
											sprintf(buffer, "IBANK> -3: Pin gresit");
											send(i, buffer, strlen(buffer), 0);
										}
									}
									j = n + 1;
								}
							}
							if (j < n + 2) {
								// numar gresit
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "IBANK> -4: Numar card inexistent");
								send(i, buffer, strlen(buffer), 0);
							}
						}
					}

					if (strcmp(token, "logout") == 0) {
						if (client_conectat == 0) {
							// nicio sesiune activa
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "IBANK> -1: Clientul nu este autentificat");
							send(i, buffer, strlen(buffer), 0);
						} else {
							client_conectat = 0;
							sprintf(buffer, "IBANK> Clientul a fost deconectat");
							send(i, buffer, strlen(buffer), 0);
						}
					}

					if (strcmp(token, "listsold") == 0) {
						if (client_conectat == 0) {
							// nicio sesiune activa
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "IBANK> -1: Clientul nu este autentificat");
							send(i, buffer, strlen(buffer), 0);
						} else {
							// send data_vec[client].sold
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "IBANK> %.2f", data_vec[client_conectat].sold);
							send(i, buffer, strlen(buffer), 0);
						}
					}

					if (strcmp(token, "transfer") == 0) {
						if (client_conectat == 0) {
							// nicio sesiune activa
							memset(buffer, 0, BUFLEN);
							sprintf(buffer, "IBANK> -1: Clientul nu este autentificat");
							send(i, buffer, strlen(buffer), 0);
						} else {
							token = strtok(NULL, delim);
							for (j = 1; j <=n; j++) {
								if (strcmp(token, data_vec[j].numar) == 0) {
									token = strtok(NULL, delim);
									dbl = atof(token);
									if (data_vec[client_conectat].sold < dbl) {
										// credit insuficient
										memset(buffer, 0, BUFLEN);
										sprintf(buffer, "IBANK> -8: Credit insuficient");
										send(i, buffer, strlen(buffer), 0);
									} else { // cere si asteapta confirmare
										memset(buffer, 0, BUFLEN);
										sprintf(buffer, "IBANK> Transfer catre %s %s? [y / n]", data_vec[j].nume, data_vec[j].prenume);
										send(i, buffer, strlen(buffer), 0);
										recv(i, buffer, sizeof(buffer), 0);
										if (*buffer == 'y' || *buffer == 'Y') {
											data_vec[client_conectat].sold -= dbl;
											data_vec[j].sold += dbl;
											memset(buffer, 0, BUFLEN);
											sprintf(buffer, "IBANK> Transfer realizat cu succes");
											send(i, buffer, strlen(buffer), 0);
										} else {
											memset(buffer, 0, BUFLEN);
											sprintf(buffer, "IBANK> -9: Operatia anulata");
											send(i, buffer, strlen(buffer), 0);
										}
									}
									j = n+1;
								}
							}
							if (j < n+2) {
								// numar gresit
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "IBANK> -4: Numar card inexistent");
								send(i, buffer, strlen(buffer), 0);
							}
						}
					}
				}
			}
		}
	}

	close(socktcp);
   
	return 0; 
}