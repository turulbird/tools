#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <tuxtxt_common.h>

extern int tuxtxt_run_ui(int pid, int demux);
extern int tuxtxt_init();
extern void tuxtxt_start(int tpid, int demux);
extern int tuxtxt_stop();
extern void tuxtxt_close();
extern void tuxtxt_handlePressedKey(int key);

int pid = 0;
int demux = 0;
int threadend = 0;

void *tuxtxtthread(void *param)
{
	tuxtxt_run_ui(pid, demux);
	threadend = 1;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	pthread_attr_t txtthreadattr;
	pthread_t txtthread;
	struct sockaddr_un cliadr, srvadr;
	int srvfd = -1;
	socklen_t len = 0;
	int key = 0, rc = -1;
	char stmp[9];    
	int bfd;

	if (argc != 3 || argv[1] == NULL || argv[2] == NULL)
	{
		ret = 1;
		goto end;
	}
	
	unlink ("/tmp/block.tmp");
	unlink ("/tmp/rc.socket");
	bfd = open("/tmp/block.tmp", O_RDWR | O_CREAT | O_TRUNC, 0777);
	close(bfd);
	
	pid = atoi(argv[1]);
	demux = atoi(argv[2]);
	
	pthread_attr_init(&txtthreadattr);
	//pthread_attr_setstacksize(&txtthreadattr, 50000);
	pthread_attr_setdetachstate(&txtthreadattr, PTHREAD_CREATE_JOINABLE);
	ret = pthread_create(&txtthread, &txtthreadattr, tuxtxtthread, NULL);
	if (ret)
	{
		printf("create thread");
		ret = 2;
		goto end;
	}
	
	srvadr.sun_family = AF_LOCAL;
	strcpy(srvadr.sun_path, "/tmp/rc.socket");
	srvfd = socket(AF_LOCAL, SOCK_STREAM, 0);     
	bind(srvfd, (struct sockaddr *)&srvadr, sizeof(srvadr));
	listen(srvfd, 10);
	len = sizeof(cliadr);
	
	while (rc < 0)
	{
		rc = accept(srvfd, (struct sockaddr *) &cliadr, &len);
		if (rc < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				continue;
			}
			printf("error in sock accept\n");
			if (srvfd > -1)
			{
				close(srvfd);
			}
			ret = 3;
			goto end;
		}	
	}
	
	while (threadend == 0)
	{
		usleep(100000);

		ret = read(rc, &stmp, sizeof("00000000"));
		if (ret == 8)
		{
			stmp[8]= '\0';
			printf("TT_KEY->: %s\n", stmp);
			sscanf(stmp, "%d", &key);
			printf("TT_KEY->: %d\n", key);
	
			tuxtxt_handlePressedKey(key);
			
			if (key == 0x1F)
			{
				break;
			}
		}
		else
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				continue;
			}
			printf("error in sock read (%d)\n", ret);
			ret = 4;
			goto end;
		}
	}

end:
	if (rc > -1)
	{
		close(rc);
	}
	if (srvfd > -1)
	{
		close(srvfd);
	}
	unlink ("/tmp/block.tmp");
	unlink("/tmp/rc.socket");
	exit(ret);
}
// vim:ts=4
