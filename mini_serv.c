/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mreidenb <mreidenb@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/10 15:51:54 by mreidenb          #+#    #+#             */
/*   Updated: 2025/02/11 02:14:21 by mreidenb         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <stdio.h>
#include <netinet/in.h>

int g_server_fd, g_nfd;
fd_set sl_read, sl_write, sl_all;
char recieve[2000000], sending[2000040]; // Very large buffer
int client_id[1024];
int id_counter = 0;

void err(char *argv)
{
	write(2, argv, strlen(argv));
	close(g_server_fd);
	exit(1);
}

void fatal_err()
{
	err("Fatal error\n");
}

void send_msg(int id)
{
	for (size_t fd = 0; fd < 1024; fd++)
		if (client_id[fd] != -1 && client_id[fd] != id && send(fd, sending, strlen(sending), 0) == -1)
			fatal_err();
}

void client_msg(int fd)
{
	if (client_id[fd] == -1 && recieve[strlen(recieve) - 1] != '\0')
		bzero(&recieve, sizeof(recieve));
	printf("Client Message \n");
	int id = client_id[fd];
	int start = 0;
	int len = strlen(recieve);
	for (size_t i = 0; i < len; i++)
	{
		if (recieve[i] == 0)
			break;
		if (recieve[i] == '\n')
		{
			//recieve[i] = '\0';
			sprintf(sending, "client %d: %s", id, &recieve[start]);
			send_msg(id);
			start = i + 1;
		}
	}
    if (start < len)
        memmove(recieve, &recieve[start], len - start);
    else
        bzero(&recieve, sizeof(recieve));
}

void server_msg(int id, char flag)
{
	if (flag == 1) // Client added
		return sprintf(sending, "server: client %d just arrived\n", id), send_msg(id);
	return sprintf(sending, "server: client %d just left\n", id), send_msg(id);
}

void add_client()
{
	int new = accept(g_server_fd, NULL, NULL);
	if (new > g_nfd)
		g_nfd = new;
	FD_SET(new, &sl_all);
	server_msg(id_counter, 1);
	client_id[new] = id_counter++;
}

void rm_client(int fd)
{
	int id = client_id[fd];
	//if (FD_ISSET(fd, &sl_all))
	FD_CLR(fd, &sl_all);
	close(fd);
	client_id[fd] = -1;
	server_msg(id, 0);
}

int main(int argc, char **argv)
{
	if (argc != 2)
		err("Wrong number of arguments\n");
	int port = atoi(argv[1]);
	g_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (g_server_fd < 0)
		fatal_err();
	g_nfd = g_server_fd;

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	serv_addr.sin_port = htons(port);

	if (bind(g_server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) // When bild fails return is NOT 0
		fatal_err();
	printf("Server Socket: %d", g_server_fd);
	if (listen(g_server_fd, 100) != 0)
		fatal_err();

	FD_ZERO(&sl_all);
	FD_SET(g_server_fd, &sl_all);

	for (size_t i = 0; i < 1024; i++)
		client_id[i] = -1;
	bzero(&recieve, sizeof(recieve));


	while (1)
	{
		sl_read = sl_write =  sl_all;
		if (select(g_nfd + 1, &sl_read, &sl_write, NULL, NULL) == -1)
			continue;
		for (size_t fd = 0; fd <= g_nfd; fd++)
		{
			if (FD_ISSET(fd, &sl_read))
			{
				if (fd == g_server_fd){
					add_client();
					break;
				}
				int recv_ret = 1;
				while (recv_ret == 1 && recieve[strlen(recieve) -1] != '\n')
				{
					recv_ret = recv(fd, recieve + strlen(recieve), 1, 0);
					if (recv_ret <= 0)
						break;
				}
				if (recv_ret != 1)
					rm_client(fd);
				client_msg(fd);
			}
		}

	}
}
