#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>

#define MAX_ROOM_SZ 3

/* Client structure */
typedef struct client_t
{
	struct sockaddr_in address;
	int sockfd;
	char name[32];
	char password[32];
	int isBlock;
	int isOnline;
	char rName[32];
} client_t;

typedef struct room
{
	client_t rClients[MAX_ROOM_SZ];
	char name[32];
	int cNum; // current number of clients
} room;

void str_trim_lf(char *arr, int length);
char *itoa(int value, char *result, int base);
void str_overwrite_stdout();
int getOption();

#endif
