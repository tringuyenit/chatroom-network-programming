#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>

#include "server-helper.h"
#include "utils.h"

#define MAX_CLIENTS 100
#define MAX_ROOMS 100
#define BUFFER_SZ 2048

static unsigned int cli_count = 0;
bool isQuit = false;

int strl = 0;
client_t *clients[MAX_CLIENTS] = {0};
room *rooms[MAX_ROOMS] = {0};
char *password;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// find a client by username
client_t *find_client_by_username(char *username)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (!strcmp(clients[i]->name, username))
				return clients[i];
		}
	}
	return NULL;
}

// login handler
client_t *login(client_t *client, char *password)
{

	if (strcmp(client->password, password) == 0)
	{
		//client->isOnline = 1;
		return client;
	}

	return NULL;
}

/* Add client to clients */
void queue_add(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// sign up handler, check if username existed then return NULL, otherwise newClient
client_t *signup(char *username, char *password)
{
	if (find_client_by_username(username) != NULL)
	{
		printf("Username exist\n");
		return NULL;
	}
	client_t *newClient = (client_t *)malloc(sizeof(client_t));
	strcpy(newClient->name, username);
	strcpy(newClient->password, password);
	newClient->isBlock = 0;
	newClient->isOnline = 0;
	queue_add(newClient);
	addNewClient(newClient);
	return newClient;
}

// check if username is existed
bool validUsername(char *name)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (strcmp(clients[i]->name, name) == 0)
			{
				return false;
			}
		}
	}
	return true;
}

// /* Send message to all online clients in room except sender */
void sendBroadCastRoom(struct client_t *cli, int confd, char *s)
{
	printf("send broadcast room. \n");

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (cli[i].name[0])
		{
			if (cli[i].sockfd != confd && cli[i].isOnline)
			{
				send_by_confd(cli[i].sockfd, s);
			}
		}
	}
}

/* Send message to all online clients except sender */
void sendBroadCast(int confd, char *s)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->sockfd != confd && clients[i]->isOnline)
			// if (clients[i]->sockfd != confd)
			{
				send_by_confd(clients[i]->sockfd, s);
			}
		}
	}
}

// send to specific user by username
void sendToUser(char *s, char *username, int confd)
{
	client_t *receiver = find_client_by_username(username);
	if (receiver == NULL)
	{

		send_by_confd(confd, "Noti: User not found!\n");
	}
	else if (receiver->isOnline == 0)
	{

		send_by_confd(confd, "Noti: User is offline. Sent failed!\n");
	}
	else
	{

		send_by_confd(receiver->sockfd, s);
		send_by_confd(confd, "Noti: Message sent!\n");
	}
}

// find a room by name, return room if found, otherwise NULL
room *findRoomByName(char *name)
{
	for (int i = 0; i < MAX_ROOMS; i++)
	{
		if (rooms[i])
		{
			if (strcmp(rooms[i]->name, name) == 0)
				return rooms[i];
		}
	}
	return NULL;
}

// send msg message handler
// if has receiver, then send private, otherwise send broadcast
void send_msg(client_t *cli, char *msg, char *receiver)
{
	char response[BUFFER_SZ];
	// send to specific user
	if (receiver[0])
	{

		sprintf(response, "%s to you: %s\n", cli->name, msg);
		sendToUser(response, receiver, cli->sockfd);
	}

	// send to everyone, if currently in a room then sendBroadcastRoom otherwise broadcast general
	else
	{

		if (cli->rName[0])
		{
			// already in room
			sprintf(response, "%s to room %s: %s\n", cli->name, cli->rName, msg);
			sendBroadCastRoom((findRoomByName(cli->rName))->rClients, cli->sockfd, response);
		}
		else
		{
			sprintf(response, "%s to everyone: %s\n", cli->name, msg);
			sendBroadCast(cli->sockfd, response);
		}
		send_by_confd(cli->sockfd, "Noti: Message sent!\n");
	}
}

// change username of a client
void changeUsername(int confd, char *username)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->sockfd == confd)
			{
				// printf("user %s found! change username", clients[i]->name);
				strcpy(clients[i]->name, username);
			}
		}
	}
}

// get all online users
void getOnlineUsers(char *response)
{
	pthread_mutex_lock(&clients_mutex);

	strcpy(response, "Online users:\n");
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (clients[i])
		{
			if (clients[i]->isOnline)
				sprintf(response, "%s%s\n", response, clients[i]->name);
		}
	}
	pthread_mutex_unlock(&clients_mutex);

	sprintf(response, "%s\r\n", response);
}

// get all online rooms with current number of room's members
void getOnlineRooms(char *response)
{
	strcpy(response, "Active rooms:\n");
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (rooms[i])
		{
			sprintf(response, "%s%s: %d users.\n", response, rooms[i]->name, rooms[i]->cNum);
		}
	}

	sprintf(response, "%s\r\n", response);
}

// add client to list rClients of room
void addClientToRoomList(client_t *list, client_t *cli)
{
	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if (!list[i].name[0])
		{
			list[i] = *cli;
			return;
		}
	}
}

// create new room
void createNewRoom(client_t *cli, char *name, char *response)
{
	// printf("create new room \n");
	for (int i = 0; i < MAX_ROOMS; ++i)
	{
		if (!rooms[i])
		{
			room *r = (room *)malloc(sizeof(room));
			strcpy(r->name, name);
			r->cNum = 1;
			r->rClients[0] = *cli;
			rooms[i] = r;
			sprintf(response, "Noti: Room '%s' created!\n", name);
			send_by_confd(cli->sockfd, response);
			return;
		}
	}
}

// add client to room by roomName, if room existed, check is fulled then add, otherwise create new room
void addClientToRoom(client_t *cli, char *name)
{
	char response[BUFFER_SZ];
	printf("add client to room \n");
	for (int i = 0; i < MAX_ROOMS; i++)
	{
		if (rooms[i])
		{
			// if room existed
			if (strcmp(rooms[i]->name, name) == 0)
			{
				// if room is fulled then notify join failed, otherwise add client to rClients and sendBroadcastRoom
				if (rooms[i]->cNum >= MAX_ROOM_SZ)
				{
					sprintf(response, "Noti: room '%s' is fulled. Cannot join!\n", name);
					send_by_confd(cli->sockfd, response);
					return;
				}

				strcpy(cli->rName, name);
				rooms[i]->cNum++;
				addClientToRoomList(rooms[i]->rClients, cli);

				sprintf(response, "Noti: joined '%s'\n", name);
				send_by_confd(cli->sockfd, response);

				sprintf(response, "Room %s: %s has joined room\n", name, cli->name);
				sendBroadCastRoom((rooms[i]->rClients), cli->sockfd, response);
				return;
			}
		}
	}
	// room is not existed, create new room
	strcpy(cli->rName, name);
	createNewRoom(cli, name, response);
}

// leave room handler
void leaveRoom(client_t *cli)
{
	room *r = findRoomByName(cli->rName);
	r->cNum--;
	removeClientFromRoom(r, cli->name);
	strcpy(cli->rName, "");
}

// evaluate message type of client
void evaluate(char *buf, client_t *cli)
{

	char response[BUFFER_SZ];
	char msg[BUFFER_SZ];
	char receiver[BUFFER_SZ];
	char keyword[BUFFER_SZ];
	// clear the buffer
	msg[0] = '\0';
	receiver[0] = '\0';
	keyword[0] = '\0';
	response[0] = '\0';

	//printf("buf = %s\n", buf);
	// send back help system
	if (!strcmp(buf, "help"))
	{
		helpSystem(response);
		send_by_confd(cli->sockfd, response);
		return;
	}
	// get the online users
	if (!strcmp(buf, "online"))
	{
		getOnlineUsers(response);
		send_by_confd(cli->sockfd, response);
		return;
	}
	/*  quit system
	 * if currently in a room then leave room firts
	 * otherwise send sendBroadCast quit system
	*/
	if (!strcmp(buf, "quit"))
	{
		isQuit = true;
		if (cli->rName[0])
			leaveRoom(cli);
		sprintf(response, "%s has left\n", cli->name);
		sendBroadCast(cli->sockfd, response);
		return;
	}

	// get current room of user, in a room or general
	if (!strcmp(buf, "wai"))
	{
		sprintf(response, "Noti: you are currently in '%s'\n", cli->rName[0] ? cli->rName : "general");
		send_by_confd(cli->sockfd, response);
		return;
	}

	// leave room
	// if currently in a room then leave room, send broadcast room about left member
	if (!strcmp(buf, "leave"))
	{
		pthread_mutex_lock(&clients_mutex);

		if (cli->rName[0])
		{
			// already in a room
			char temp[50];
			strcpy(temp, cli->rName);
			room *r = findRoomByName(cli->rName);

			leaveRoom(cli);

			sprintf(response, "Room %s: user %s has left\n", temp, cli->name);
			sendBroadCastRoom(r->rClients, cli->sockfd, response);
		}
		send_by_confd(cli->sockfd, "Noti: you are in 'general' now\n");
		pthread_mutex_unlock(&clients_mutex);

		return;
	}

	// get online rooms list with number of members
	if (!strcmp(buf, "rooms"))
	{
		pthread_mutex_lock(&clients_mutex);
		getOnlineRooms(response);
		send_by_confd(cli->sockfd, response);
		pthread_mutex_unlock(&clients_mutex);
		return;
	}

	// case msg, room, username
	// get keyword, msg and receiver with format
	sscanf(buf, "%s \" %[^\"] \"%s", keyword, msg, receiver);
	//printf("keyword = %s, msg = %s, last = %s.\n", keyword, msg, receiver);

	// case msg:
	// if nothing in double quotes then invalid
	// otherwise send_msg handle if public or private msg
	if (!strcmp(keyword, "msg"))
	{
		if (!msg[0])
			send_by_confd(cli->sockfd, "Invalid command\n\r\n");
		else
		{
			pthread_mutex_lock(&clients_mutex);
			send_msg(cli, msg, receiver);
			pthread_mutex_unlock(&clients_mutex);
		}
		return;
	}

	// case change username
	// if nothing in double quotes then invalid
	// otherwise check if new username is valid then change and broadcas, or noti invalid username
	if (!strcmp(keyword, "username"))
	{
		if (!msg[0])
			send_by_confd(cli->sockfd, "Invalid command\n\r\n");
		else
		{
			pthread_mutex_lock(&clients_mutex);
			char temp[30] = "";
			strcpy(temp, cli->name);
			if (!validUsername(msg))
				send_by_confd(cli->sockfd, "Noti: Username is used by other user!\n\r\n");
			else
			{
				changeUsername(cli->sockfd, msg);
				// notice to other users
				sprintf(response, "Noti: user '%s' has changed to '%s'\n\r\n", temp, msg);
				sendBroadCast(cli->sockfd, response);
				send_by_confd(cli->sockfd, "Noti: Username changed!\n\r\n");
			}
			pthread_mutex_unlock(&clients_mutex);
		}
		return;
	}

	// case join a room
	// if currently in a room then noti please leave before join
	// otherwise handle join room
	if (!strcmp(keyword, "room"))
	{
		pthread_mutex_lock(&clients_mutex);

		if (!msg[0])
			send_by_confd(cli->sockfd, "Invalid command\n\r\n");
		else
		{
			if (cli->rName[0])
				send_by_confd(cli->sockfd, "Noti: you are currently in a room. Please leave before join other room.\n\r\n");
			else
				addClientToRoom(cli, msg);
		}

		pthread_mutex_unlock(&clients_mutex);
		return;
	}

	// if not in above case then invalid command
	else
	{
		send_by_confd(cli->sockfd, "Invalid command\n\r\n");
		return;
	}
}

/* Handle all communication with the client */
void *handle_client(void *arg)
{
	char buff_out[BUFFER_SZ], temp[67], username[32], password[32];
	int leave_flag = 0, isLogin = 0;

	char controlReceive[1] = {0};
	cli_count++;
	client_t *cli = (client_t *)arg;
	client_t *client = NULL;

	// get first request, login, sign up or exit
	do
	{
		memset(username, 0, 32);
		memset(password, 0, 32);
		memset(controlReceive, 0, 1);

		// Name
		if (recv(cli->sockfd, temp, 67, 0) <= 0)
			leave_flag = 1;
		// client sent exit request
		else if (temp[0] == '3')
		{
			printf("User is left.\n");
			leave_flag = 1;
		}
		else
		{
			// controlReceive = 2 is sign up case, = 1 is login case
			printf("client send:%s\n", temp);
			strcpy(controlReceive, strtok(temp, " "));
			strcpy(username, strtok(NULL, " "));
			strcpy(password, strtok(NULL, " "));

			if (controlReceive[0] == '2')
			{
				//sign up
				client_t *newClient = signup(username, password);
				if (newClient == NULL)
					send_by_confd(cli->sockfd, "Sign up failed. Username existed!\n");
				else
				{
					newClient->address = cli->address;
					newClient->sockfd = cli->sockfd;
					send_by_confd(cli->sockfd, "Sign up successful. Please login to system!\n");
				}
			}
			else if (controlReceive[0] == '1')
			{
				// login case
				// check if account is exist
				// check password and isBlock status
				// then noti if login success or not
				client = find_client_by_username(username);
				if (client == NULL)
				{
					printf("login fail\n");
					send_by_confd(cli->sockfd, "Login failed. Account is not exist!\n");
				}
				else
				{
					client = login(client, password);
					if (client == NULL)
					{
						printf("incorrect pasword\n");
						send_by_confd(cli->sockfd, "Login failed. Password is incorrect!\n");
					}
					else if (client->isBlock == 1)
					{
						printf("account blocked\n");
						send_by_confd(cli->sockfd, "Login failed. Account is block!\n");
					}

					else if (client->isOnline == 1)
					{

						send_by_confd(cli->sockfd, "Login failed. Account is current login!\n");
					}
					else
					{
						printf("login success\n");
						send_by_confd(cli->sockfd, "Login success!\n");

						client->address = cli->address;
						client->sockfd = cli->sockfd;
						client->isOnline = 1;
						sprintf(buff_out, "Noti: %s has joined general\n", client->name);
						sendBroadCast(cli->sockfd, buff_out);

						isLogin = 1;
						break;
					}
				}
			}
		}
	} while (!(controlReceive[0] == '1' && isLogin) || !leave_flag);

	bzero(buff_out, BUFFER_SZ);

	// logged in case, handle new incomming message by function evaluate
	if (isLogin)
	{

		while (!isQuit)
		{
			if (leave_flag)
				break;

			int receive = recv(client->sockfd, buff_out, BUFFER_SZ, 0);

			if (receive > 0)
			{
				if (strlen(buff_out) > 0)
				{
					str_trim_lf(buff_out, strlen(buff_out));
					evaluate(buff_out, client);
					printf("%s -> %s\n", client->name, buff_out);
				}
			}
			else if (receive == 0 || strcmp(buff_out, "quit") == 0)
			{
				sprintf(buff_out, "%s has left\n", client->name);
				printf("%s", buff_out);

				sendBroadCast(client->sockfd, buff_out);

				leave_flag = 1;
			}
			else
			{
				printf("ERROR: -1\n");
				leave_flag = 1;
			}

			bzero(buff_out, BUFFER_SZ);
		}

		// client exit, set online status = 0, detach thread
		client->isOnline = 0;
		pthread_detach(pthread_self());
	}
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	/* Socket settings */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	/* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
	// if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
	{
		perror("ERROR: setsockopt failed");
		return EXIT_FAILURE;
	}

	/* Bind */
	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

	/* Listen */
	if (listen(listenfd, 10) < 0)
	{
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	printf("=== WELCOME TO THE CHATROOM ===\n");
	// fill data client to clients
	takeUserData(clients);

	while (1)
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if ((cli_count + 1) == MAX_CLIENTS)
		{
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;

		/* Fork thread */

		pthread_create(&tid, NULL, &handle_client, (void *)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}
