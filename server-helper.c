
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "server-helper.h"
#include "utils.h"
#define BUFFER_SZ 2048
#define MAX_CLIENTS 100


void helpSystem(char *response)
{
    strcpy(response, "Usage:\n");
    sprintf(response, "%smsg \"text\" : send the msg to all online clients \n", response);
    sprintf(response, "%smsg \"text\" username : send the msg to a specific client\n", response);
    sprintf(response, "%sonline : get the username of all the clients online\n", response);
    sprintf(response, "%susername \"text\": change username\n", response);
    sprintf(response, "%sroom \"room-name\": join a chatroom\n", response);
    sprintf(response, "%srooms : get name of all active rooms\n", response);
    sprintf(response, "%sleave : leave current room, go to general\n", response);
    sprintf(response, "%swai : where am i (get current room)\n", response);
    sprintf(response, "%squit : exit the chatroom\n\r\n", response);
}

// send msg to a client by connection fd
void send_by_confd(int confd, char *response)
{
    if (write(confd, response, strlen(response)) < 0)
    {
        perror("ERROR: send failed");
    }
}

void print_client_addr(struct sockaddr_in addr)
{
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}


void printClient(client_t *client)
{
	printf("name :%s; password: %s; isOnline:%d; isBlock:%d\n", client->name, client->password, client->isOnline, client->isBlock);
}


// add new client to file account.txt
void addNewClient(client_t *client)
{
	FILE *fp = fopen("account.txt", "a");

	char *temp = (char *)malloc(sizeof(char));
	fprintf(fp, "\n");
	strcat(temp, client->name);
	strcat(temp, " ");
	strcat(temp, client->password);
	strcat(temp, " ");
	char isBlock[1];
	itoa(client->isBlock, isBlock, 10);
	strcat(temp, isBlock);

	temp[strlen(temp)] = 0;
	fprintf(fp, "%s", temp);

	fclose(fp);
}


// get user data from file "account.txt" and add to clients
void takeUserData(client_t* clients[])
{

	char data[BUFFER_SZ] = {0};
	FILE *fp = fopen("account.txt", "r");

	int i = 0;
	while (fgets(data, BUFFER_SZ, fp) != NULL)
	{
		client_t *client = (client_t *)malloc(sizeof(client_t));
		char username[32], password[32];
		int isBlock;
		strcpy(username, strtok(data, " "));
		strcpy(password, strtok(NULL, " "));
		isBlock = atoi(strtok(NULL, " "));
		strcpy(client->name, username);
		strcpy(client->password, password);
		client->isBlock = isBlock;
		client->isOnline = 0;
		clients[i] = client;
		//printf("clients[%d] = %s.\n", i, clients[i]->name);
		i++;
		memset(data, 0, BUFFER_SZ);
	}
	fclose(fp);
}

// update user in file
void updateUserData(client_t* clients[])
{
	FILE *fp = fopen("account.txt", "w");
	for (int i = 0; i < sizeof(clients); ++i)
	{
		char *temp = (char *)malloc(sizeof(char));
		strcat(temp, clients[i]->name);
		strcat(temp, " ");
		strcat(temp, clients[i]->password);
		strcat(temp, " ");
		char isBlock[1];
		itoa(clients[i]->isBlock, isBlock, 10);
		strcat(temp, isBlock);
		fprintf(fp, "%s", temp);
	}
	fclose(fp);
}


// remove client from rClients of room
void removeClientFromRoom(room *r, char *name)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (r->rClients[i].name[0])
		{
			if (!strcmp(r->rClients[i].name, name))
			{
				printf("found client to leave room.\n");
				r->rClients[i].sockfd = 0;
				strcpy(r->rClients[i].name, "");
				break;
			}
		}
	}
}