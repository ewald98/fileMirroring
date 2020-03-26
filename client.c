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
#include <arpa/inet.h>

#define PORT 6112

#define MSG_SIZE 1024

#define CLIENT_FOLDER_PATH "/home/ewi/client/"
#define COMPARE_FILE_PATH "/home/ewi/pad/compareFile.txt"
#define USER_PATH "/home/ewi"

char files[1000][300];
int filesIndex;

char directories[20][100];
int directoriesNo;

void printma() {

    for (int i = 0; i < filesIndex; i++)
    {
		printf("line %d: ", i);
        for (int j = 0; j < strlen(files[i]); j++)
            printf("%c", files[i][j]);
        printf("\n");
    }
}

int system(const char *command);

void removeFilesLeft()
{
	printf("Removing files left... (%d files)\n", filesIndex);
	int j;
	char path[256];

	for (int i = 0; i < filesIndex; i++)
	{
		memset(path, '\0', 256);
		j = 0;
		while (files[i][j]!=';')
	  	{
			path[j] = files[i][j];
		 	j++;
    	}
		printf("Attempting to delete %s...", path);

		chdir("client");
		if(unlink(path)<0)
		{
			perror("unlink error!");
			exit(1);
		}
		printf("Delete succesful\n");
	}
	system("find /home/ewi/client -empty -type d -delete");
}

void deleteRow(int r) {

    if (r != filesIndex - 1)
        for (int j = 0; j < 300; j++)
            files[r][j] = files[filesIndex - 1][j];
    filesIndex--;
}

void createFilesMatrix(int exists)
{
	if (exists == 1) {
	    FILE *arr;
	    char c;
	    arr = fopen(COMPARE_FILE_PATH,"r");

	    while(fgets(files[filesIndex++],sizeof(char) * 300,arr)) {
	        printf("%s\n", files[filesIndex]);
	    }

		filesIndex--;	// last line is empty
	    fclose(arr);
	}
}

int isUpToDate(char *fileName, char *fileDate, int compareFilefd)
{
	int j, k;
    char data[300], path[256], date[30];
	memset(data, '\0', 300);
	memset(path, '\0', 256);
	memset(date, '\0', 30);

    for (int i = 0; i < filesIndex; i++)
    {
		j = 0, k = 0;
        while (files[i][j]!=';') {
            path[j] = files[i][j];
            j++;
        }
        path[j] = '\0';
        j++;

        printf("Path:  %s\n", path);

		printf("COMPARE#%d:\n%s\n%s\n=======\n", i, fileName, path);
        if(strcmp(fileName, path) == 0)
        {
			printf("HIT");
            while(files[i][j]!='\n') {
                date[k] = files[i][j];
                j++,k++;
            }
            date[k] = '\0';

            if(strcmp(date, fileDate) == 0) {
                strcpy(data, fileName);
                data[strlen(fileName)] = ';';
                data[strlen(fileName) + 1] = '\0';
                strcat(data, fileDate);
                write(compareFilefd, (void *)data, strlen(data));
                deleteRow(i);
                return 1;
            }
            else
            {
                strcpy(data, fileName);
                data[strlen(fileName)] = ';';
                data[strlen(fileName) + 1] = '\0';
                strcat(data, fileDate);
                write(compareFilefd, (void *)data, strlen(data));
                deleteRow(i);
                return 0;
            }
        }
    }
    strcpy(data, fileName);
    data[strlen(fileName)] = ';';
    data[strlen(fileName) + 1] = '\0';
    strcat(data, fileDate);
    write(compareFilefd, (void *)data, strlen(data));
    return 0;
}

char *removeFileName(char *s) {
    int n = strlen(s);

    char *s2;
    s2 = (char *) malloc(strlen(s));
    int done = 0;

    for (int j = n - 1; j >= 0; j--) {
        if (done == 1)
            s2[j] = s[j];

        if (s[j] == '/' && done == 0) {
            s2[j] = '\0';
            done = 1;
        }
    }

    return s2;
}

char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}

char * getTheLastDirectory(char *path)
{
	int i=0;
	char *res;
	res = malloc(sizeof(char));
	strrev(path);

	while(path[i]!='/')
		{res[i] = path[i]; i++;}
	res[i] = '\0';
	strrev(res);
	return res;
}

void creatyDirectory(char *path)
{
	int n;
	DIR *dir = opendir(path);
	char *pch;
	char pathCopy[strlen(path)];
	if(dir == NULL)
	{
		strcpy(pathCopy, path);
		pch= getTheLastDirectory(pathCopy);
		directories[directoriesNo][0] = '/';
		strcat(directories[directoriesNo],pch);
		directoriesNo++;
		int len = strlen(path)-strlen(pch)-1;
		path[len] = '\0';
		creatyDirectory(path);

	}
	int newPathLen = strlen(path)+strlen(directories[directoriesNo-1])+1;
	char newPath[newPathLen];
	strcpy(newPath,path);
	strcat(newPath,directories[directoriesNo-1]);
	directoriesNo--;
    // printf("CREATYDIR:  %d\n", newPathLen);
	newPath[newPathLen] = '\0';
	mkdir(newPath,S_IRWXU);
	strcpy(path, newPath);
	if(dir!=NULL)
	{
		int cdir = closedir(dir);
		if(cdir == -1)
		{
			perror("closedir error");
			exit(4);
		}
	}

}

void firstDirectory(char *path)
{
    chdir(USER_PATH);

	DIR *firstDir = opendir(path);
	if(ENOENT == errno)
		mkdir(path,S_IRWXU);//crate main directoru, FileMirroring
	else{
		int cdir = closedir(firstDir);
		if(cdir == -1)
		{
			perror("closedir error");
			exit(1);
		}
	}
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

void checkCompareFile() {
    int exists = 1;
    if (access(COMPARE_FILE_PATH, F_OK) == -1) {
        // file doesn't exist
		printf("COMPARE FILE NONEXISTENT, CREATED\n");
        int fd;
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        if (!(fd = creat(COMPARE_FILE_PATH, mode))) {
            perror("create error");
            exit(3);
        }
        close(fd);
        exists = 0;
    }

    createFilesMatrix(exists);
    printf("==============\n");
    printma();
    printf("==============\n");
}

void initiateUpdate(int sock) {
    // prerequisites
    checkCompareFile();
    firstDirectory("client");

    // send first msg
    char msg[MSG_SIZE];

	memset(msg, '\0', MSG_SIZE);
    msg[0] = 'i';

    if (stream_write(sock, msg, MSG_SIZE) < 0) {
        perror("stream_write error");
        exit(5);
    }

    int r;
    if ((r = stream_read(sock, msg, MSG_SIZE)) < 0) {
        perror("read error");
        exit(5);
    }

    if (msg[0] == 'a') {
        printf("ack done\n");

        return;
    }
    else
    {
        perror("ack error");
        exit(6);
    }
}

void finishUpdate(int sock) {

    removeFilesLeft();
    printf("Finished update, client side ^^\n");

    close(sock);
}

void performUpdate(int sockfd) {
    char fileInfo[MSG_SIZE], fileRequest[MSG_SIZE], fileSegment[MSG_SIZE];

    int r;

	int compareFilefd;
	compareFilefd = open(COMPARE_FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
	if (compareFilefd < 0)
	{
		perror("open file error");
		exit(3);
	}

    while (1) {
        if ((r = stream_read(sockfd, fileInfo, MSG_SIZE) < 0))
        {
            perror("read error");
            exit(5);
        }

        if (fileInfo[0] == 'd')    // done => updateCompleted
        {
            break;
        }
        else if (fileInfo[0] == 'u')    // update => performingUpdate
        {
            // printf("FILE:");
            // printf("%s \n%s \n%s \n", fileInfo, fileInfo+4, fileInfo+260);

            printf("FILEINDEX: %d\n", filesIndex);

            if (!isUpToDate(fileInfo+4, fileInfo+260, compareFilefd)) {
                // aici vine verificarea de la Anca
                if (strchr(fileInfo + 4 , '/') != NULL) {
                    char *fileInfo3 = removeFileName(fileInfo + 4);
                    char fileInfo2[256];
                    strcpy(fileInfo2, "client/");
                    strcat(fileInfo2, fileInfo3);
                    creatyDirectory(fileInfo2);
                    directoriesNo = 0;
                    for (int i = 0; i < 20; i++)
                        memset(directories[i], '\0', 100);
                    // char fileInfo4[256] = "mkdir -p ";
                    // strcat(fileInfo4, fileInfo3);
                    // system(fileInfo4);
                }

                int fd, segment_len;
                mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

                char absoluteFilePath[270];
                strcpy(absoluteFilePath, CLIENT_FOLDER_PATH);
                strcat(absoluteFilePath, fileInfo + 4);
                printf("ABSOLUTE FILE PATH:%s\n", absoluteFilePath);
                if (!(fd = creat(absoluteFilePath, mode))) {
                    perror("create error");
                    exit(3);
                }

                memset(fileRequest, '\0', MSG_SIZE);
                fileRequest[0] = 'r';

                if (stream_write(sockfd, fileRequest, MSG_SIZE) < 0) {
                    perror("stream_write error");
                    exit(11);
                }

                while (1) {
                    if ((r = stream_read(sockfd, fileSegment, MSG_SIZE) < 0)) {
                        perror("read error");
                        exit(5);
                    }
                    // printf("read %s, %s, %s\n", fileSegment, fileSegment+2, fileSegment+4);

                    if (fileSegment[0] == 'e')
                        break;

                    if (fileSegment[0] != 's') {
                        perror("incorrect step");
                        exit(2);
                    }

                    segment_len = decodeDataSize(fileSegment + 2);

                    if (write(fd, fileSegment + 4, segment_len) == -1)
                    {
                        perror("write error\n");
                        exit(4);
                    }
                }

                close_file(fd);
            }

        }
        else
        {
            perror("no such step");
            exit(13);
        }
    }
}

int main(void)
{
    int sock, r;
    struct sockaddr_in server_address;
    char *buffer2 = "CLIENT HERE";
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Client socket error");
        exit(1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        exit(2);
    }

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connect error");
        exit(3);
    }

    initiateUpdate(sock);
    performUpdate(sock);
    finishUpdate(sock);


    return 0;
}
