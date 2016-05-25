#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <error.h>
#include <errno.h>

/***************************************************************
 * Function: [刘专]
 * Parameters:
 * Description: 
****************************************************************/
int writeFile(const char * _fileName, char * _buf, unsigned int _bufLen)
{  
    FILE * fp = NULL;
    if( NULL == _buf || _bufLen <= 0 ) return -1;
  
    fp = fopen(_fileName, "wb");
  
    if( NULL == fp )
    {  
        return -1;  
    }  
  
    fwrite(_buf, _bufLen, 1, fp);
  
    fclose(fp);
    fp = NULL;
    return 0;      
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/***************************************************************
 * Function: [刘专]
 * Parameters:
 * Description: 
****************************************************************/
ssize_t	readn(int fd, void * vptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;
	char * ptr = NULL;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
    {
		if ( (nread = read(fd, ptr, nleft)) < 0)
        {
			if (errno == EINTR)
            {
				nread = 0;
            }
            else
            {
                return -1;
            }
		}
        else if (nread == 0)
        {
    		break;
        }

		nleft -= nread;
		ptr   += nread;
	}

	return (n - nleft);
}

/***************************************************************
 * Function: [刘专]
 * Parameters:
 * Description: 
****************************************************************/
int tcp_channel(char * take_server_ip, int take_port, char * buf, int buf_len)
{
    int sockfd;
	struct sockaddr_in dest;

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		printf("Error: socket create failure!\n");
		return -1;
	}

	dest.sin_family = AF_INET;
	dest.sin_addr.s_addr = inet_addr(take_server_ip);
	dest.sin_port = htons(take_port);

	if( connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0 )
	{
		printf("Error: connect to remote is failure!\n");
		return -1;
	}

    int n = readn(sockfd, buf, buf_len);
    if( n < 0 )
    {
        printf("[testFSMOD] Error: ReadN Error, Read Bytes %d, %s:%d:%s\n", n, __FILE__, __LINE__, __FUNCTION__);
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return 0;
}

/***************************************************************
 * Function: [刘专]
 * Parameters:
 * Description: 
****************************************************************/
int main()
{
    char * buf = NULL;
    int retWrite = 0;
    int fileContentLen = 898;
    buf = (char *)malloc(sizeof(char)*fileContentLen);
    memset(buf, 0, sizeof(buf));

    char fileNameWrite[32] = "./copy.png";
    tcp_channel("192.168.199.225", 9000, buf, 898);

    writeFile(fileNameWrite, buf, 898);

    return 0;
}

