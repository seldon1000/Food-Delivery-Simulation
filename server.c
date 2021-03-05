#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct Client
{
	struct sockaddr_in clientAddr;
	int index;
	int sockID;
	int len;
} clients[1024];

struct Ristoranti
{
	int index;
	int menu; // indica il cliente che ha chiesto di vedere il menu
	char name[1024];
} ristoranti[1024];

pthread_t thread[1024];

int clientsCount = 0;
int ind = 0;

char rist[1024]; // stringa della lista dei ristoranti che verrà inviata ai clienti

void *doNetworking(void *ClientDetail)
{
	struct Client *clientDetail = (struct Client *)ClientDetail;

	char type[2];
	char data[1024];

	recv(clientDetail->sockID, type, sizeof(char), 0); // aspetto di sapere se si tratta di un rist o di un cliente

	if (strcmp(type, "r") == 0) // se  è un ristorante
	{
		int currRist = ind++;

		char temp[1024];

		recv(clientDetail->sockID, data, sizeof(data), 0); // ricevi il nome

		strcpy(ristoranti[currRist].name, data);		  // salvo il nome
		ristoranti[currRist].index = clientDetail->index; //salva l'index nel tuo array ordinato di ristoranti
		ristoranti[currRist].menu = -1;

		printf("Il ristorante '%s' si e' connesso.\n", ristoranti[currRist].name);

		snprintf(temp, sizeof(temp), "[%d]%s\n", currRist + 1, ristoranti[currRist].name);
		strcat(rist, temp); // aggiorno la lista dei ristoranti

		while (1) // resto in ascolto del ristorante corrente, la ricezione dai ristoranti è centralizzata qui
		{
			memset(data, 0, sizeof(data));
			recv(clientDetail->sockID, data, sizeof(data), 0); // ricevo qualcosa dal ristorante

			if (data[0] == '[') // se ho ricevuto il suo menu, invialo al cliente che lo ha chiesto
			{
				send(clients[ristoranti[currRist].menu].sockID, data, sizeof(data), 0);
				ristoranti[currRist].menu = -1; // permetti ad altri clienti di chiedere il menu di questo ristorante
			}
			else if (data[0] == 'r') // invece ho ricevuto l'id di un rider
			{
				char rID[1024];
				strcpy(rID, data + 1); // salvo questo id

				memset(data, 0, sizeof(data));
				recv(clientDetail->sockID, data, sizeof(data), 0); // prima di dedicarti a un altro ordine, aspetta l'id del cliente interessato

				send(clients[atoi(data)].sockID, rID, sizeof(rID), 0); // invia l'id del rider al cliente
			}
			else // altrimenti se il rider ha effettuato la consegna, ricevo l'id del cliente servito conferma la consegna al cliente che la stava aspettando
			{
				send(clients[atoi(data)].sockID, "y", sizeof(char), 0);

				printf("L'ordine del cliente numero %d e' stato consegnato.\n", atoi(data));

				close(clients[atoi(data)].sockID);
			}
		}
	}
	else if (strcmp(type, "c") == 0) // se sei un cliente
	{
		printf("Il cliente numero '%d' si e' connesso.\n", clientDetail->index);

		int scelta;

		char clientID[1024];
		char ordine[2][1024];

		snprintf(data, sizeof(data), "%d", clientDetail->index);
		strcpy(clientID, data);

		while (1) // finche il cliente non effettua l'ordine
		{
			send(clientDetail->sockID, rist, sizeof(rist), 0); // invio la lista dei ristoranti disponibili al cliente

			memset(data, 0, sizeof(data));
			recv(clientDetail->sockID, data, sizeof(data), 0); // ricevo la scelta del ristorante

			scelta = atoi(data) - 1;

			printf("Il cliente numero %d vuole vedere il menu del ristorante '%s'.\n", clientDetail->index, ristoranti[scelta].name);

			while (ristoranti[scelta].menu != -1) // scelto il ristorante, aspetto di poter inviare il menu
				;
			ristoranti[scelta].menu = clientDetail->index; // nessun altro cliente può chiedere il menu del ristorante se il cliente corrente non l'ha ancora ricevuto

			send(clients[ristoranti[scelta].index].sockID, "menu", 4 * sizeof(char), 0); // chiedo al rist selezionato il menu

			memset(data, 0, sizeof(data));
			recv(clientDetail->sockID, data, sizeof(data), 0); // ricevo l'ordine

			if (strcmp("-1", data) != 0) // se ricevo -1 significa che il cliente vuole tornare tornare alla lista dei rist, altrimenti esco
				break;
		}

		printf("Il cliente numero %d ha effettuato un ordine presso il ristorante '%s'.\n(Ordine: %s)\n\n", clientDetail->index, ristoranti[scelta].name, data);

		strcpy(ordine[0], clientID);
		strcpy(ordine[1], data);

		send(clients[ristoranti[scelta].index].sockID, ordine, sizeof(ordine), 0); // invio ordine

		pthread_exit(0);
	}
}

int main()
{
	struct sockaddr_in serverAddr;
	int listenFD = socket(AF_INET, SOCK_STREAM, 0);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		exit(-1);

	if (listen(listenFD, 1024) == -1)
		exit(-1);
	printf("Il server e' stato avviato sulla porta 8080 ...........\n\n");

	while (1)
	{
		clients[clientsCount].sockID = accept(listenFD, (struct sockaddr *)&clients[clientsCount].clientAddr, &clients[clientsCount].len);
		clients[clientsCount].index = clientsCount;

		pthread_create(&thread[clientsCount], NULL, doNetworking, (void *)&clients[clientsCount]); // ogni cliente o rist che si connette avvio un thread e torno ad ascoltare

		clientsCount++;
	}
}
