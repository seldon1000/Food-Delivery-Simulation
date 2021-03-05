#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("usage: program <restaurant IP address> <port>\n");
		exit(-1);
	}

	struct sockaddr_in serverAddr;

	int in_consegna = 0;
	int clientSocket = socket(PF_INET, SOCK_STREAM, 0);

	char id[1024];
	char data[1024];
	char scelta[2];

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[2]));
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

	printf("Inserire l'id personale: ");
	scanf("%s", &id);

	if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		return 0;
	printf("\nConnessione al ristorante stabilita.\n\n");

	while (1)
	{
		while (in_consegna)
		{
			memset(data, 0, sizeof(data));
			recv(clientSocket, data, sizeof(data), 0); // se ho accettato una consegna ricevo id del cliente

			printf("Richiesta accettata. Il numero del cliente e' %s.\n", data);
			printf("Inviare y a consegna ultimata.\n");

			scanf("%*s"); // attendo un qualsiasi input da riga di comando

			send(clientSocket, "y", sizeof(char), 0); // ordine consegnato

			in_consegna = 0;
		}

		while (!in_consegna)
		{
			printf("Attendo richieste...\n\n");

			memset(data, 0, sizeof(data));
			recv(clientSocket, data, sizeof(data), 0); // ricevo la richiesta

			printf("Ho ricevuto una richiesta.\n%s\nAccettare? (y, n) ", data);
			scanf("%s", &scelta);

			if (strcmp("y", scelta) == 0)
			{
				send(clientSocket, id, sizeof(id), 0); // la accetto lo comunico a rist

				memset(data, 0, sizeof(data));
				recv(clientSocket, data, sizeof(data), 0);

				if ((strcmp("y", data) == 0)) // se sono stato il piu veloce ricevo y
					in_consegna = 1;
				else
				{
					printf("Consegna gia presa in carico da un altro rider. %s\n", data); // se non ho rusposto per primo
					break;
				}
			}
			else
				send(clientSocket, "n", sizeof(char), 0); // non la accetto e continuo ad aspettare richieste
		}
	}
}
