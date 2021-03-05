#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct Client
{
	int index;
	int sockID;
	int len;
	struct sockaddr_in clientAddr;
} clients[1024]; // tutti i client che si connettono (ovvero i rider)

struct timeval tv; // timeout per la select

pthread_t ordini[1024];
pthread_t main_t;									 // thread principale che genera gl ialtri thread
pthread_mutex_t mutexID = PTHREAD_MUTEX_INITIALIZER; // mutua esclusione per ottenere l'id del cliente

int clientsCount = 0; //numero di rider connessi
int i_ordini = 0;
int cs;			   // globale  sarebbe la socket del server
int maxSockFD = 0; // massimo file descriptor da analizzare nella select
int riderOccupati[1024];

float prezzi[6];
float prezziPizza[6] = {3.5, 3, 4.5, 4.5, 5, 5};
float prezziPesce[6] = {5, 12, 10.5, 8, 6.5, 7};

char *csIP;
char menu[1024]; //questa serve , è il menu in unica stringa, varia da ristorante a risotarante
char menust[6][100];
char *menuPizza = "[1]MARGHERITA\t3.5\n[2]MARINARA\t3\n[3]CAPRICCIOSA\t4.5\n[4]ORTOLANA\t4.5\n[5]SALSICCIE E FRIARIELLI\t5\n[6]4 STAGIONI\t5"; //questa serve , è il menu in unica stringa, varia da ristorante a risotarante
char *menuPizzast[] = {"MARGHERITA  ", "MARINARA  ", "CAPRICCIOSA  ", "ORTOLANA  ", "SALSICCIE E FRIARIELLI  ", "4 STAGIONI  "};
char *menuPesce = "[1]CALAMARI FRITTI\t5\n[2]SPAGHETTI VONGOLE\t12\n[3]RISO PESCATORA\t10.5\n[4]ORATA\t8\n[5]ALICI FRITTE\t6.5\n[6]SCAMPI\t7"; //questa serve , è il menu in unica stringa, varia da ristorante a risotarante
char *menuPescest[] = {"CALAMARI FRITTI  ", "SPAGHETTI VONGOLE  ", "RISO PESCATORA  ", "ORATA  ", "ALICI FRITTE  ", "SCAMPI  "};
char ordine[2][1024];

void resetMultiplex(fd_set *localRiders, int x) // resetto il multiplex, necessario ad ogni esecuzione della select
{
	FD_ZERO(localRiders);

	for (int i = 0; i < clientsCount; i++)
	{
		if (riderOccupati[i] != x)
			FD_SET(clients[i].sockID, localRiders);
	}
}

void *startordine()
{
	char clientID[1024];
	char richiesta[1024];

	char data[1024];

	if (strcmp(ordine[0], "menu") == 0) // se ricevo una richiesta di menu
	{
		send(cs, menu, 1024, 0); // invio menu

		pthread_exit(0); // chiudo il thread
	}

	strcpy(clientID, ordine[0]);
	strcpy(richiesta, ordine[1]);

	memset(ordine, 0, sizeof(ordine));

	int scelta;

	float totale = 0;

	char ordineRicevuto[1024];

	for (int k = 0; k < strlen(richiesta); k++) // itero su ordine per carattere
	{
		scelta = richiesta[k] - '0' - 1;
		totale += prezzi[scelta]; // sommo al totale
		snprintf(data, sizeof(data), "%s %.2f TOT:%.2f \n", menust[scelta], prezzi[scelta], totale);
		strcat(ordineRicevuto, data); // temp è ogni riga temporanea es : margherita  3.00  totale 3.00
	}

	printf("Ordine ricevuto:\n%s\n", ordineRicevuto);

	fd_set localRiders; // file descriptor dei rider da monitorare nella select

	int sel;
	int riderRes = -1;  // conterrà il rider che risponde per primo
	int antiSpam[1024]; // antiClelia

	char riderResID[100] = "r"; // conterrà l'id personale del rider che risponde per primo

	for (int i = 0; i < 1024; i++)
		antiSpam[i] = 0;

	while (1) // continua finchè l'ordine non è stato affidato a qualcuno
	{
		resetMultiplex(&localRiders, 0);

		if ((sel = select(maxSockFD + 1, NULL, &localRiders, NULL, &tv)) <= 0) // vedi quanti rider sono pronti a ricevere
			continue;

		for (int i = 0; i < clientsCount; i++)
		{
			if (FD_ISSET(clients[i].sockID, &localRiders) && antiSpam[i] == 0) // se il rider è pronto e se non ha già rifiutato l'ordine in precedenza
			{
				riderOccupati[i] = 0;												// rider occupato, non riceverà altre richieste per ora
				send(clients[i].sockID, ordineRicevuto, sizeof(ordineRicevuto), 0); // gli invio l'ordine
			}
			else if (FD_ISSET(clients[i].sockID, &localRiders)) // se un rider aveva già rifiutato l'ordine, non dovrai aspettare una risposta da lui
				sel--;
		}

		while (sel > 0) // aspetta una risposta da tutti i rider che hanno ricevuto l'ordine
		{
			resetMultiplex(&localRiders, 1);

			sel -= select(maxSockFD + 1, &localRiders, NULL, NULL, NULL); // quanti rider mi hanno risposto

			for (int i = 0; i < clientsCount; i++)
			{
				if (FD_ISSET(clients[i].sockID, &localRiders) && i != riderRes)
				{
					memset(data, 0, sizeof(data));
					recv(clients[i].sockID, data, sizeof(data), 0); // ricevi la risposta del rider

					if (strcmp("n", data) != 0) // se è y allora ha accettato
					{
						if (riderRes == -1) // se è il primo a rispondere
						{
							send(clients[i].sockID, "y", sizeof(char), 0); // dagli il via libera per cosegnare l'ordine
							strcat(riderResID, data);
							riderRes = i;
						}
						else // se non è il primo
						{
							send(clients[i].sockID, "n", sizeof(char), 0); // digli che qualcuno ha già preso in carico l'ordine
							riderOccupati[i] = 1;						   // non è più occupato, riceverà ordini da altri clienti
						}
					}
					else
					{
						riderOccupati[i] = 1;
						antiSpam[i] = 1; // se ha risposto n non deve più ricevere questo ordine
					}
				}
			}
		}

		if (sel == 0 && riderRes != -1) // se hai gestito tutto le risposte dei rider e hai trovato quello a cui affidare l'ordine
			break;
	}

	printf("Il rider numero %s ha preso in carico l'ordine.\n", riderResID + 1);

	send(clients[riderRes].sockID, clientID, sizeof(clientID), 0); // invio id cliente a rider

	send(cs, riderResID, sizeof(riderResID), 0); // invio id rider a server
	sleep(1);
	send(cs, clientID, sizeof(clientID), 0); // invio id cliente a server

	recv(clients[riderRes].sockID, data, sizeof(data), 0); // ricevo l'ok dell'ordine consegnato

	riderOccupati[riderRes] = 1; // il rider ha consegnato e potrà ricevere nuovi ordini

	send(cs, clientID, sizeof(clientID), 0); // invio ok a server

	printf("Il rider numero %s ha consegnato l'ordine.\n", riderResID + 1);

	pthread_exit(0);
}

void *routine() // thread principale per parlare col server
{
	struct sockaddr_in serverAddr;

	int scelta;

	char name[1024];
	char data[2][1024];

	cs = socket(AF_INET, SOCK_STREAM, 0);

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);
	serverAddr.sin_addr.s_addr = inet_addr(csIP);

	printf("Inserire il nome del ristorante: ");
	fgets(name, sizeof(name), stdin);
	strtok(name, "\n");

	printf("\nMenu da offrire:\n[1]Pizza\n[2]Pesce\n");
	scanf("%d", &scelta);

	if (scelta == 1)
	{
		strcpy(menu, menuPizza);

		for (int i = 0; i < 6; i++)
		{
			strcpy(menust[i], menuPizzast[i]);
			prezzi[i] = prezziPizza[i];
		}
	}
	else
	{
		strcpy(menu, menuPesce);

		for (int i = 0; i < 6; i++)
		{
			strcpy(menust[i], menuPescest[i]);
			prezzi[i] = prezziPesce[i];
		}
	}

	if (connect(cs, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		exit(-1);

	printf("\nConnessione al server stabilita............\n\n");

	send(cs, "r", sizeof(char), 0);	 // invio r per dire che sono un rist
	send(cs, name, sizeof(name), 0); // invio nome

	while (1) // ricezione centralizzata dei messaggi dal server
	{
		memset(data, 0, sizeof(data));
		recv(cs, data, sizeof(data), 0); // sempre in ascolto

		strcpy(ordine[0], data[0]);
		strcpy(ordine[1], data[1]);

		pthread_create(&ordini[i_ordini], NULL, startordine, &data); // nuovo thread per ogni ordine e torno ad ascoltare su recv

		i_ordini++;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("usage: program <server IP address> <port>\n");
		exit(-1);
	}

	struct sockaddr_in mAddr;

	int serverSocket = socket(PF_INET, SOCK_STREAM, 0);

	csIP = argv[1];

	mAddr.sin_family = AF_INET;
	mAddr.sin_port = htons(atoi(argv[2]));
	mAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	tv.tv_sec = 1; // la server aspetterà al più un secondo per vedere quanti rider sono pronti a ricevere, così non resta bloccata per sempre nel caso abbiano rifiutato tutti
	tv.tv_usec = 0;

	for (int i = 0; i < 1024; i++) // array dei rider occupati su un ordine
		riderOccupati[i] = 1;

	if (bind(serverSocket, (struct sockaddr *)&mAddr, sizeof(mAddr)) == -1)
	{
		printf("errore\n");
		exit(-1);
	}

	if (listen(serverSocket, 1024) == -1)
	{
		printf("errore\n");
		exit(-1);
	}

	pthread_create(&main_t, NULL, routine, NULL); //pima di entrare nel while creo un main thread per comunicare col server

	while (1)
	{
		clients[clientsCount].sockID = accept(serverSocket, (struct sockaddr *)&clients[clientsCount].clientAddr, &clients[clientsCount].len);
		clients[clientsCount].index = clientsCount;

		if (clients[clientsCount].sockID > maxSockFD) // quando un rider si connette, il suo file descriptor diventa il massimo per la select
			maxSockFD = clients[clientsCount].sockID;

		printf("Rider connesso.\n");

		clientsCount++; // stessa cosa del server ogni rider che entra l oaccetto e creo un thread per lui
	}
}
