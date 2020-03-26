#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define PORT 6112

#define MSG_SIZE 1024
#define DATA_SIZE 1020

#define SERVER_FOLDER "/home/ewi/fileMirroring"

char *getRelativePath(char *source) {
	int n = 4, i = 0;
	while (n > 0) {
		if (source[i] == '/')
			n--;
		i++;
	}
	return source + i;
}

void close_file(int du)
{
	if (close(du) != 0)
	{
		perror("close error\n");
		exit(3);
	}
}

void setDataSize(unsigned int x, char *s)
{
    for (int i = 1; i >= 0; i--)
    {
        unsigned char b = (x >> 8 * i) & 0xFF;
        s[i] = b;
    }
}

int decodeDataSize(char *s)
{
    return (unsigned char) s[0] + 256 * (unsigned char) s[1];
}

char *getFileCreationTime(char *path) {
	struct stat attr;
	stat(path, &attr);
	return ctime(&attr.st_mtime);
}

int stream_write(int sockfd, char *buf, int len) {
    int nwr;
    int remaining = len;

    while (remaining > 0) {
        if (-1 == (nwr = write(sockfd, buf, remaining)))
            return -1;
        remaining -= nwr;
        buf += nwr;
    }
    return len - remaining;
}

int stream_read(int sockfd, char *buf, int len) {
    int nread;
    int remaining = len;

    while (remaining > 0) {
        if (-1 == (nread = read(sockfd, buf, remaining)))
            return -1;
        if (nread == 0)
            break;
        remaining -= nread;
        buf += nread;
    }
    return len - remaining;
}

void ackUpdate(int client_fd) {
    char msg[MSG_SIZE];
    int r;

    if ((r = stream_read(client_fd, msg, MSG_SIZE) < 0))
    {
        perror("read error1");
        exit(5);
    }

    if (msg[0] == 'i') // 'iu' = initiateUpdateCode
    {
		memset(msg, '\0', MSG_SIZE);
        msg[0] = 'a';

        if (stream_write(client_fd, msg, MSG_SIZE) < 0) {
            perror("stream_write error");
            exit(5);
        }
        printf("init done\n");

        return;
    }
    else
    {
        perror("init error");
        exit(6);
    }
}

void performUpdate(int client_fd, char *path) {

    DIR *dir;
    struct dirent *entry;
    struct stat st;
    int ret;

    if (!(dir = opendir(path))) {
        perror("opendir error");
        exit(2);
    }

    while (entry = readdir(dir)) {

        if ((strcmp(".", entry->d_name) == 0) || (strcmp("..", entry->d_name) == 0))
            continue;

        char filePath[256];
        sprintf(filePath, "%s/%s", path, entry->d_name);
        if ((ret = lstat(filePath, &st)) == -1) {
            printf("errno error! %s\n", strerror(errno));
            perror("lstat error");
            exit(10);
        }


        if ((S_ISREG(st.st_mode) || S_ISLNK(st.st_mode)) && (!S_ISDIR(st.st_mode)))
        {
        	printf("%s\n", getRelativePath(filePath));
		    char fileInfo[MSG_SIZE], fileRequest[MSG_SIZE], fileSegment[MSG_SIZE];
			int r;

			memset(fileInfo, '\0', MSG_SIZE);
		    fileInfo[0] = 'u';

            strcpy(fileInfo + 4, getRelativePath(filePath));
            strcpy(fileInfo + 4 + 256, getFileCreationTime(filePath));
            if (stream_write(client_fd, fileInfo, MSG_SIZE) < 0) {
                perror("stream_write error");
                exit(11);
            }

			if ((r = stream_read(client_fd, fileRequest, MSG_SIZE) < 0)) {
				perror("read error");
				exit(5);
			}
			// printf("File requested: %s", fileInfo + 4);
			if (fileRequest[0] == 'r') {
				// open file
				int du;
				if (!(du = open(filePath, O_RDONLY))) {
					perror("open file error");
					exit(14);
				}

				char buffer[1020];
				int n;
				memset(fileSegment, '\0', MSG_SIZE);
				fileSegment[0] = 's';	// segment

				while (n = read(du, buffer, 1020)) {
					strcpy(fileSegment + 4, buffer);
					setDataSize(n, fileSegment + 2);

					if (stream_write(client_fd, fileSegment, MSG_SIZE) < 0) {
						perror("stream_write error");
						exit(5);
					}
				}
				if (n == -1) {
					perror("read error");
					exit(9);
				}

				close_file(du);
				// END OF FILE
				memset(fileSegment, '\0', MSG_SIZE);
				fileSegment[0] = 'e';
				if (stream_write(client_fd, fileSegment, MSG_SIZE) < 0) {
					perror("stream_write error");
					exit(5);
				}

			}
        }
        if (S_ISDIR(st.st_mode))
            performUpdate(client_fd, filePath);
    }

    int cdir = closedir(dir);
    if (cdir == -1) {
        perror("closedir error");
        exit(12);
    }
}

void updateCompleted(int client_fd) {
	char msg[MSG_SIZE];

	memset(msg, '\0', MSG_SIZE);
    msg[0] = 'd';

    if (stream_write(client_fd, msg, MSG_SIZE) < 0) {
        perror("stream_write error");
        exit(11);
    }

}

int main(void)
{
    int server_fd,fd;
    struct sockaddr_in address;
    int addr_len = sizeof(address);
    int opt = 1;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Server socket error");
        exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(2);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, addr_len) < 0) {
        perror("Bind error");
        exit(3);
    }

    if (listen(server_fd, 2) < 0) {
        perror("listen error");
        exit(4);
    }

    while (1) {
        if ((fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len)) < 0) {
            perror("accept error");
            exit(5);
        }
        if(fork()==0)
        { // proces fiu
            close(server_fd);

            ackUpdate(fd);
            performUpdate(fd, SERVER_FOLDER);
			updateCompleted(fd);

            exit(0);
        }
        close(fd);
    }

    return 0;
}
