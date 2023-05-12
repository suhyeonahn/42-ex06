#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define	ARGS	"Wrong number of arguments\n"
#define	FATAL	"Fatal error\n"
#define BUFF	1024

int		maxfd, sockfd, id = 0;
int		client_ids[1024];
char	toWrite[BUFF];
fd_set	readS, writeS;

void	error(char *msg)
{
	if (sockfd > 2)
		close(sockfd);
	write(2, msg, strlen(msg));
	exit(1);
}

void sendAll(int senderfd) {
    for (int i = 2; i <= maxfd; i++) {
        if(FD_ISSET(i, &writeS) && i != senderfd)
            send(fd, toWrite, strlen(toWrite), 0);
    }
}

int main(int ac, char **av)
{
    struct sockaddr_in	servaddr, cliaddr;
    int					clifd;
	socklen_t			len;

    if (ac != 2)
		error(ARGS);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error(FATAL);

    memset(&servaddr, 0, sizeof(servaddr)); 
	servaddr.sin_family		 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
	servaddr.sin_port		 = htons(atoi(av[1]));

    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		error(FATAL);

    if (listen(sockfd, 4096) < 0)
		error(FATAL);

    len = sizeof(cliaddr);
	FD_ZERO(&readS);
    FD_ZERO(&writeS);
	FD_SET(sockfd, &readS);
    FD_SET(sockfd, &writeS);
    maxfd = sockfd;

    while (1)
	{
		if (select(maxfd + 1, &readS, &writeS, NULL, NULL) < 0)
			continue;

		for (int fd = 2; fd <= maxfd; fd++)
		{
			if (FD_ISSET(fd, &readS))
			{
                memset(&toWrite, 0, BUFF);
                if (fd == sockfd)
		        {
			        if ((clifd = accept(sockfd, (struct sockaddr *)&cliaddr, &len)) < 0)
				        error(FATAL);
                    FD_SET(clifd, &readS);
                    FD_SET(clifd, &writeS);
			        maxfd = (clifd > maxfd) ? clifd : maxfd;
                    client_ids[clifd] = id++;

                    sprintf(toWrite, "server: client %d just arrived\n", client_ids[clifd]);
                    sendAll(clifd);
                }
                else 
                {
                    int nbytes = 1;
                    while (nbytes == 1 && toWrite[strlen(toWrite) - 1] != '\n')
					    nbytes = recv(fd, toWrite + strlen(toWrite), 1, 0);
				    if (nbytes <= 0)
                    {
                        memset(&toWrite, 0, BUFF);
                        sprintf(toWrite, "server: client %d just left\n", client_ids[fd]);
                        sendAll(fd);
                        FD_CLR(fd, &readS);
                        FD_CLR(fd, &writeS);
                        close(fd);
                        client_ids[fd] = 0;
                    }
                    else
                    {
                        sprintf(toWrite, "client %d: %s", client_ids[fd], toWrite);
                        sendAll(fd);
                    }
                }
            }
		}
	}
	return (0);
}