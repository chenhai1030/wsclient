#include "easywsclient.hpp"
#include <assert.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <fstream>

#include<sys/stat.h>
#include <fcntl.h>


#define WS_STREAM_URL "ws://172.17.7.11:8000/chat/stream"
#define WS_URL "ws://172.17.7.11:8000/chat/connect"
//#define WS_URL "ws://59.172.252.67:8100/chat/connect"

#define TMP_FILE "/tmp/wsdata"
#define FIFO "/tmp/stdout_fifo"
#define SCREEN_FILE "/tmp/speed.png"

#define SPEED_TEST_CMD "SpeedTest"

#define BUF_SIZE_MAX (4*1024)
#define BUF_SIZE_LINE 512 


static int fun_system(const char * cmd);

using easywsclient::WebSocket;
static volatile WebSocket::pointer ws = NULL;
static volatile WebSocket::pointer ws_stream = NULL;


static void handle_sig(int sig)
{
	if (sig == SIGCHLD){
		int pid;
		int status;
		printf("recv SIGCHLD\n");
		while((pid = waitpid(-1, &status, WNOHANG)) > 0){
		}
	}
}

static void get_speedtest_screen()
{
	char cmd_buf[128];			
    std::vector<uint8_t> streambuf;
	pid_t son;
	FILE *stream_fp = NULL;
	int status;
	unsigned long filesize = -1;
		
	sprintf((char*)cmd_buf, "screencap -p %s",  SCREEN_FILE);
	son = fun_system(cmd_buf);
	if (son > 0){
		waitpid(son,&status,WUNTRACED);

		using namespace std;
		ifstream f(SCREEN_FILE, ios::binary);
		unsigned char c;
		while(f >> c) streambuf.push_back(c);
		ws_stream = WebSocket::from_url(WS_STREAM_URL);
		if (ws_stream != NULL)
			ws_stream->sendBinary(streambuf);
	}
}

static pid_t fun_system(const char * cmd)
{
	pid_t pid;
	int status;
//	char command[512] = {0};
//	char **args;
//	int i,len;
//	int tokens=0;
//
//	i=0;
//	strncpy(command, cmd, sizeof(cmd));
//	while((command[i]!=0)&&((command[i]==' ')||(command[i]=='\t'))) i++;
//	while(command[i] != 0)
//	{
//		while((command[i]!=0)&&(command[i]!=' ')&&(command[i] != '\t')) i++;
//		tokens++;
//		while((command[i]!=0)&&((command[i]==' ')||(command[i] == '\t'))) i++;
//	}
//
//	args=(char **)malloc((tokens+1)*sizeof(char *));
//	len = i;
//	i = 0;
//	tokens = 0;
//	do{
//		while ((command[i]!=0)&&((command[i]==' ')||(command[i]=='\t'))) i++;
//		args[tokens++]=command+i;
//		while ((command[i]!=0)&&((command[i]!=' ')&&(command[i]!='\t'))) i++;
//		command[i++]=0;
//	}while(i<len);
//
	if((pid = fork())<0){
		pid = -1;
	}else if (pid == 0){
		int fd = open(FIFO, O_WRONLY);
		dup2(fd, 1);

		execl("/system/bin/sh", "sh", "-c", cmd, (char *)0);
		//execvp(args[0], args);
		exit(127); 
	}
	signal(SIGCHLD, handle_sig);
	return pid;
}

static void *fun_send_msg_process(void *)
{
	char line[BUF_SIZE_LINE];
	char buf[BUF_SIZE_MAX];
	FILE* fp = NULL;
	unsigned int len = 0;
	
	unlink(FIFO);
	int res = mkfifo(FIFO, 0777);
	if (res != 0)
		printf("fifo error!!%d \n", res);

	memset(line, 0, sizeof(line));
	while(true){
		fp = fopen(FIFO, "rb");
		if (fp!= NULL){
			while( fgets( line, sizeof(line), fp) != NULL && (len < BUF_SIZE_MAX - BUF_SIZE_LINE) ){
				strncpy(&buf[0] + len, line, strlen(line));
				len += strlen(line);
			}
			if (ws != NULL && ws->getReadyState() != WebSocket::CLOSED){
				//printf("> %s", line);
				ws->send(buf);	
				len = 0;
				memset(buf, 0, sizeof(buf));
			}
			usleep(10000);
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
	if (ret != 0){
		return -1;
	}

	ret = pthread_create(&task_id, &thread_attr, &fun_send_msg_process, NULL);
	if (ret != 0){
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
	if (argc > 1){
		ws->send(argv[1]);
	}else{
		ws->send("hello");
	}

    while (ws->getReadyState() != WebSocket::CLOSED) {
        WebSocket::pointer wsp = &*ws; // <-- because a unique_ptr cannot be copied into a lambda
        ws->poll(300);
		if (ws->getReadyState() != WebSocket::CLOSED){
			ws->dispatch([&cmd](const std::string & message) {
				printf(">>> %s\n", message.c_str());
				//sprintf((char*)cmd, "%s > %s",  message.c_str(), TMP_FILE);
				sprintf((char*)cmd, "%s",  message.c_str());
		//        if (message == "world") { wsp->close(); }
			});
			if (strlen(cmd) > 0){
				if (child_pid > 0){
					kill(child_pid, SIGKILL);
					printf("kill :%d \r\n", child_pid);
					sleep(1);
				}
				
				if (strncmp(cmd, SPEED_TEST_CMD, sizeof(cmd)) == 0){
//					get_speedtest_screen();
				}else{
					child_pid = fun_system(cmd);
					printf("child_pid %d \r\n", child_pid);
					memset(cmd, 0, sizeof(cmd));
				}
			}
		}
		else{
			is_restart = true;
		}

		if (ws->getHeartbeat() >= 2 || is_restart == true){
			ws->close();
			ws->poll();
			ws->poll();

			delete ws;
			ws = NULL;
			printf("Heart beat error ---> new ws create! \n");
			ws = WebSocket::from_url(WS_URL);
			ws->send(argv[1]);
			count = 0;
			is_restart = false;
		}
		
		if (count++ % 20 == 1){
			ws->sendPing();
			ws->incHeartbeat();
		}
    }
    // N.B. - unique_ptr will free the WebSocket instance upon return:
    return 0;
}
