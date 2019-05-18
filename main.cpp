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

#include <sys/stat.h>
#include <fcntl.h>

#include "upload.h"
#include "common.h"



char g_mac_str[32] = {0};
static int fun_system(const char * cmd);
static void get_speedtest_screen();

using easywsclient::WebSocket;
volatile WebSocket::pointer ws = NULL;

static int g_s_heart_freq = 20;

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

static void upload_file(int file_type, int file_num, const char * file_path)
{
	upload_param_t *p_upload_param = NULL; 

	p_upload_param = (upload_param_t *)malloc(sizeof(upload_param_t));
	if (p_upload_param != NULL){
		p_upload_param->m_file_type = file_type;
		p_upload_param->m_file_num = file_num;
		if (file_path != NULL){
			p_upload_param->m_p_file_path = (char *)malloc(strlen(file_path)+1);
			if (p_upload_param->m_p_file_path){
				memset(p_upload_param->m_p_file_path, 0 , strlen(p_upload_param->m_p_file_path)+1);
				strncpy(p_upload_param->m_p_file_path, file_path, strlen(file_path)+1);	
			}
		}
	}

	upload_file_task(p_upload_param);
}

static void start_speedtest_activity()
{
	system("input keyevent BACK");
	system(SPEED_TEST_ACTIVITY_CMD);	
	upload_file(FILE_TYPE_IMG, SCREEN_CAP_NUM, SCREEN_SPEED_TEST_FILE);
}

static void start_linktest_activity()
{
	system("input keyevent BACK");
	system(LINK_TEST_ACTIVITY_CMD);	
	upload_file(FILE_TYPE_IMG, SCREEN_CAP_NUM, SCREEN_LINK_TEST_FILE);
}

static void start_upload_file(const char * file_path)
{
	upload_file(FILE_TYPE_FILE, 1, file_path);
}

static void get_speedtest_screen()
{
	char cmd_buf[128];			
	using namespace std;
    std::vector<uint8_t> streambuf;
	pid_t son;
	FILE *stream_fp = NULL;
	int status;
	unsigned long filesize = -1;
	WebSocket::pointer ws_stream = NULL;

	sprintf((char*)cmd_buf, "screencap -p %s",  SCREEN_FILE);
	son = fun_system(cmd_buf);
	if (son > 0){
		waitpid(son,&status,WUNTRACED);

		ifstream f(SCREEN_FILE, ios::binary);
		unsigned char c;
		while(f >> c) streambuf.push_back(c);
		ws_stream = WebSocket::from_url(WS_STREAM_URL);
		printf("chenhai test---> \n");
		if (ws_stream != NULL){
			ws_stream->sendBinary(streambuf);
		}
	}
}

static pid_t fun_system(const char * cmd)
{
	pid_t pid;
	int status;

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
		loop:
			while(fgets( line, sizeof(line), fp) != NULL && (len < BUF_SIZE_MAX - BUF_SIZE_LINE)){
				strncpy(&buf[0] + len, line, strlen(line));
				len += strlen(line);
			}
			if (ws != NULL && ws->getReadyState() != WebSocket::CLOSED){
				ws->send(buf);	
				len = 0;
				memset(buf, 0, sizeof(buf));
				usleep(1000*10);	
			}
			if (fgets( line, sizeof(line), fp) != NULL){
				strncpy(&buf[0] + len, line, strlen(line));
				len += strlen(line);
				goto loop;
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
	if (ret != 0){
		return -1;
	}

	ret = pthread_create(&task_id, &thread_attr, fun_send_msg_process, NULL);
	if (ret != 0){
		pthread_attr_destroy(&thread_attr);
		return -1;	
	}
	return 0;
}

static void get_macaddr(const char * str)
{
	char *p_cus = g_mac_str;
	char *p_colon = p_cus;
	strncpy(g_mac_str, str+8, 17);
	
	while(*p_cus != '\0'){
		if (*p_cus != ':'){
			*p_colon = *p_cus;
			*p_colon++;
			*p_cus++;
		}else{
			p_cus++;
		}
	}
	*p_colon = *p_cus;
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
	ws = WebSocket::from_url(WS_URL_CONNECT);
    assert(ws);

	handle_msg_init();
	if (argc > 1){
		get_macaddr(argv[1]);
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
				if (strncmp(cmd, "logcat", 6) == 0){
					g_s_heart_freq = 20;	
				}
				if (child_pid > 0){
					kill(child_pid, SIGKILL);
					printf("kill :%d \r\n", child_pid);
					sleep(1);
				}
				
				if (strncmp(cmd, SPEED_TEST_CMD, sizeof(cmd)) == 0){
					start_speedtest_activity();	
				}else if (strncmp(cmd, LINK_TEST_CMD, sizeof(cmd)) == 0){
					start_linktest_activity();	
				}else {
					if (strncmp (cmd, GET_FILE_CMD, strlen(GET_FILE_CMD)) == 0){
						char * p = strtok(cmd, " ");
						char * path = strtok(NULL, " ");
						start_upload_file((const char *)path);		
					}else{
						if (strlen(cmd) > 7 &&  strncmp (cmd, "busybox", 7) == 0){
							system(cmd);
						}else{
							child_pid = fun_system(cmd);
						}
						printf("child_pid %d \r\n", child_pid);
					}
				}
				memset(cmd, 0, sizeof(cmd));
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
			printf("Heart beat error %d---> new ws create! \n", is_restart);
			printf("url: %s \n", WS_URL_CONNECT);
			ws = WebSocket::from_url(WS_URL_CONNECT);
			ws->send(argv[1]);
			count = 0;
			is_restart = false;
		}
		
		if (count++ % g_s_heart_freq == 1){
			ws->sendPing();
			ws->incHeartbeat();
		}
    }
    // N.B. - unique_ptr will free the WebSocket instance upon return:
    return 0;
}
