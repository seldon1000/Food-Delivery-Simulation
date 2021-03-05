#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("usage: program <server IP address>\n");
		exit(-1);
	}

	struct sockaddr_in serverAddr;

	int sockFD = socket(AF_INET, SOCK_STREAM, 0);

	char data[1024];
	char scelta[100] = "-1";
	char ordine[100] = "";

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

	if (connect(sockFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
		exit(-1);
	printf("Connessione con il server stabilita.\n");

	send(sockFD, "c", sizeof(char), 0); // invio c pe dire che sono un cliente

	while (strcmp("-1", scelta) == 0) // se seleziono -1 vuol dire che voglio tornare alla lista dei ristoranti
	{
		memset(data, 0, sizeof(data));
		recv(sockFD, data, sizeof(data), 0); // ricevo la lista dei rist
		printf("\nRistoranti disponibili\n%s\n", data);

		printf("Seleziona un ristorante: ");
		scanf("%s", scelta); // scelgo un rist

		send(sockFD, scelta, sizeof(char), 0); // invio scelta

		printf("\nSelezionare una o più pietanze (cliccare invio per ogni pietanza)\n(inviare 0 per completare l'ordine)\n(inviare -1 per tornare alla scelta del ristorante)\n\n");

		recv(sockFD, data, sizeof(data), 0); // ricevo il menu da server
		printf("%s\n", data);

		memset(ordine, 0, sizeof(ordine));

		while (1)
		{
			scanf("%s", scelta); //faccio l'ordine, ogni numero è una pietanza se premo 0  ho concluso e invio il mio ordine

			if (strcmp(scelta, "0") == 0)
			{
				send(sockFD, ordine, sizeof(ordine), 0); // INVIO UN ORDINE DEL TIPO   132243

				printf("Ordine inviato.\n\n");

				break;
			}
			else if (strcmp(scelta, "-1") == 0)
			{
				send(sockFD, scelta, sizeof(scelta), 0); //dico al server che voglio di nuovo la lista dei ristoranti

				break;
			}

			strcat(ordine, scelta);
		}
	}

	memset(data, 0, sizeof(data));
	recv(sockFD, data, sizeof(data), 0); // ricevo l'id del rider

	printf("Il rider numero %s ha preso in carico il mio ordine.\n", data);

	recv(sockFD, NULL, sizeof(char), 0); // aspetto che il rider effuttui la consegna

	printf("\nOrdine consegnato.\n");

	close(sockFD);
}
