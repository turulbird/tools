#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/unistd.h>

int main(int argc, char *argv[])
{
	int ms = 0;

	if (argc != 2)
	{
		printf("usage: %s millisec\n", argv[0]);
		exit(1);
	}
  
	ms = atoi(argv[1]);
	ms = ms * 1000;
	usleep(ms);
	exit(0);
}
//vim:ts=4
