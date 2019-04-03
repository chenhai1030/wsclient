#ifndef _UPLOAD_H_
#define _UPLOAD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <curl/curl.h>
#include <unistd.h>

#define FILE_TYPE_IMG 1
#define FILE_TYPE_FILE 2

#define SCREEN_CAP_NUM 3

typedef struct{
	int m_file_type;
	int m_file_num;
	char * m_p_file_path;
}upload_param_t;

int upload_file_task(upload_param_t *p_params);
#endif
