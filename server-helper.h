
#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

void helpSystem(char *respone);
void send_by_confd(int confd, char* response);
void print_client_addr(struct sockaddr_in addr);
void printClient(client_t *client);

void addNewClient(client_t *client);
void takeUserData(client_t* clients[]);
void updateUserData(client_t* clients[]);
void removeClientFromRoom(room *r, char *name);



#endif