#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#define	ARGS	"Wrong number of arguments\n"
#define	FATAL	"Fatal error\n"

int         sockfd, fd_max, id = 0;
fd_set      activeS, readS, writeS;
int         cli_ids[65000];
char        *msg_buffer[65000];
char        buffer[1025];
char        send_info[50];

int extract_message(char **buf, char **msg)
{
    char    *newbuf;
    int    i;

    *msg = 0;
    if (*buf == 0)
        return (0);
    i = 0;
    while ((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
            if (newbuf == 0)
                return (-1);
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
        i++;
    }
    return (0);
}

char *str_join(char *buf, char *add)
{
    char    *newbuf;
    int        len;

    if (buf == 0)
        len = 0;
    else
        len = strlen(buf);
    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
    if (newbuf == 0)
        return (0);
    newbuf[0] = 0;
    if (buf != 0)
        strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
}

void    send_all(int sender, char *str)
{
    for (int fd = 0; fd <= fd_max; fd++)
    {
        if (FD_ISSET(fd, &writeS) && fd != sender)
            send(fd, str, strlen(str), 0);
    }
}

void	error(char *msg)
{
	if (sockfd > 2)
		close(sockfd);
	write(2, msg, strlen(msg));
	exit(1);
}

int main(int argc, char **argv)
{
	struct sockaddr_in  servaddr, cliaddr;
    int                 clifd;
    socklen_t           len;

     if (ac != 2)
		error(ARGS);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error(FATAL);
     
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
    servaddr.sin_port = htons(atoi(argv[1])); 
    
    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		error(FATAL);

    if (listen(sockfd, 1024) < 0)
		error(FATAL);

    FD_ZERO(&activeS);
    FD_SET(sockfd, &activeS);
    fd_max = sockfd;

    len = sizeof(cliaddr);

    while (1)
    {
        readS = writeS = activeS;

        if (select(fd_max + 1, &_fds_read, &_fds_write, 0, 0) < 0)
            error(FATAL);

         for (int fd = 0; fd <= fd_max; fd++)
        {
            if (FD_ISSET(fd, &readS))
			{
                if (fd == sockfd)
                {
                    if ((clifd = accept(sockfd, (struct sockaddr *)&cli, &len)) < 0)
                        error(FATAL);

                    cli_ids[clifd] = id++;
                    msg_buffer[clifd] = NULL;

                    sprintf(send_info, "server: client %d just arrived\n", cli_ids[clifd]);
                    send_all(clifd, send_info);

                    fd_max = (clifd > fd_max) ? clifd : fd_max;
                    FD_SET(clifd, &activeS);
                }
                else
                {
                    int    ret = recv(fd, buffer, 1024, 0);
                    if (ret <= 0)
                    {
                        sprintf(send_info, "server: client %d just left\n", cli_ids[fd]);
                        send_all(fd, send_info);
                        
                        free(msg_buffer[fd]);
                        msg_buffer[fd] = NULL;

                        close(fd);
                        FD_CLR(fd, &activeS);
                    }
                    else
                    {
                        buffer[ret] = '\0';
                        msg_buffer[fd] = str_join(msg_buffer[fd], buffer);
                        for (char *msg = NULL; extract_message(&msg_buffer[fd], &msg);)
                        {
                            sprintf(send_info, "client %d: ", cli_ids[fd]);
                            send_all(fd, send_info);
                            send_all(fd, msg);

                            free(msg);
                            msg = NULL;
                        }
                    }
                }
            }
        }
    }
    return (0);
}
