#include "easywsclient.hpp"
#include <assert.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <atomic>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

//#define WS_URL "ws://172.17.7.11:8000/chat/connect"
#define WS_URL "ws://59.172.252.67:8100/chat/connect"
#define TMP_FILE "/tmp/wsdata"

#include<sys/stat.h>
#include <fcntl.h>
#define FIFO "/tmp/stdout_fifo"



using easywsclient::WebSocket;
static volatile WebSocket::pointer ws = NULL;


static int fun_system(const char * cmd)
{
	pid_t pid;
	int status;

	if((pid = fork())<0)
	{
		pid = -1;
	}
	else if (pid == 0)
	{
		int fd = open(FIFO, O_WRONLY);
//		printf("write %d \n", fd);
		dup2(fd, 1);
		
		execl("/system/bin/sh", "sh", "-c", cmd, (char *)0);
		exit(127); 
	}
	return pid;
}

static void *fun_send_msg_process(void *)
{
	char line[1024];
	FILE* fp = NULL;
	
	unlink(FIFO);
	int res = mkfifo(FIFO, 0777);
	if (res != 0)
		printf("fifo error!!%d \n", res);

	while(true)
	{
		fp = fopen(FIFO, "r");
		if (fp!= NULL)
		{
			while( fgets( line, sizeof(line), fp) != NULL )
			{
				if (ws != NULL && ws->getReadyState() != WebSocket::CLOSED)
				{
					//printf("> %s", line);
					ws->send(line);	
				}
				usleep(1000*10);
			}
		}
		usleep(1000*500);	
	}	
	return NULL;
}

static int handle_msg_init()
{
	pthread_t task_id;
	int ret = 0;
	pthread_attr_t thread_attr;
	int priority;

	ret = pthread_attr_init(&thread_attr);
	if (ret != 0)
	{
		return -1;
	}

	ret = pthread_create(&task_id, &thread_attr, &fun_send_msg_process, NULL);
	if (ret != 0)
	{
		pthread_attr_destroy(&thread_attr);
		return -1;	
	}
	return 0;
}

int main(int argc,char *argv[])
{
	char cmd[255];
	int fd[2];
	pid_t child_pid = 0;
	static unsigned int count = 0;
	bool is_restart = false;

	memset(cmd, 0, sizeof(cmd));

    //std::unique_ptr<WebSocket> ws(WebSocket::from_url(WS_URL));
	//ws = WebSocket::from_url_no_mask(WS_URL);
	ws = WebSocket::from_url(WS_URL);
    assert(ws);

	handle_msg_init();
	if (argc > 1)
	{
		ws->send(argv[1]);
	}
	else
	{
		ws->send("hello");
	}

    while (ws->getReadyState() != WebSocket::CLOSED) {
        WebSocket::pointer wsp = &*ws; // <-- because a unique_ptr cannot be copied into a lambda
        ws->poll(300);
		if (ws->getReadyState() != WebSocket::CLOSED)
		{
			ws->dispatch([&cmd](const std::string & message) {
				printf(">>> %s\n", message.c_str());
				//sprintf((char*)cmd, "%s > %s",  message.c_str(), TMP_FILE);
				sprintf((char*)cmd, "%s",  message.c_str());
		//        if (message == "world") { wsp->close(); }
			});
			if (strlen(cmd) > 0)
			{
				if (child_pid > 0)
				{
					kill(child_pid, SIGKILL);
					printf("kill :%d \r\n", child_pid);
					sleep(1);
				}
				child_pid = fun_system(cmd);
				printf("child_pid %d \r\n", child_pid);
				memset(cmd, 0, sizeof(cmd));
			}
		}
		else{
			is_restart = true;
		}

		if (ws->getHeartbeat() >= 2 || is_restart == true)
		{
			ws->close();
			usleep(1000);
			delete ws;
			ws = NULL;
			printf("Heart beat error ---> new ws create! \n");
			ws = WebSocket::from_url(WS_URL);
			ws->send(argv[1]);
			count = 0;
			is_restart = false;
		}
		
		if (count++ % 10 == 1)
		{
			ws->sendPing();
			ws->incHeartbeat();
		}
    }
    // N.B. - unique_ptr will free the WebSocket instance upon return:
    return 0;
}
