/* 
* The imaginary form we'll fill in looks like:
 *  
 *  <form method="post" enctype="multipart/form-data" action="examplepost.cgi">
 *		Enter file: <input type="file" name="sendfile" size="40">
 *		Enter file name: <input type="text" name="filename" size="30">
 *      <input type="submit" value="send" name="submit">
 *  </form>
 */
#include "easywsclient.hpp"
#include "upload.h"
#include "common.h"

static char time_str[64] = {0};

using easywsclient::WebSocket;
extern WebSocket::pointer ws;

static char* getCurrentTimeStr()
{
	time_t t = time(NULL);
	memset(time_str, 0, sizeof(time_str));
	strftime(time_str, sizeof(time_str) - 1, "%Y%m%d%H%M", localtime(&t));     //年-月-日 时-分
	return time_str;
}

static int upload(const char * url, const char * file, const char * file_field, bool is_noexpectheader)
{
	CURL *curl;
	CURLcode res; 
	char f_field[128] = {0};

	struct curl_httppost *formpost = NULL;  
	struct curl_httppost *lastptr = NULL;
	struct curl_slist *headerlist = NULL;
	static const char buf[] = "Expect:";

	curl_global_init(CURL_GLOBAL_ALL);

	strncpy(f_field, file_field, strlen(file_field));		

	/* Fill in the file upload field */ 
	curl_formadd(&formpost,
				 &lastptr,
				 CURLFORM_COPYNAME, f_field,
				 CURLFORM_FILE, file,
				 CURLFORM_END);

	/* Fill in the filename field */ 
	curl_formadd(&formpost,
				 &lastptr,
				 CURLFORM_COPYNAME, "filename",
				 CURLFORM_COPYCONTENTS, file, 
				 CURLFORM_END);

	curl = curl_easy_init();
	/* initialize custom header list (stating that Expect: 100-continue is not
	 *      wanted */ 
	headerlist = curl_slist_append(headerlist, buf);
	if (curl){
		/* what URL that receives this POST */ 
		curl_easy_setopt(curl, CURLOPT_URL, url);
		if (is_noexpectheader)
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res));

		curl_easy_cleanup(curl);
		curl_formfree(formpost);
		curl_slist_free_all(headerlist);
	}

	return 0;
}

static void *fun_upload_process(void * arg)
{
	char cmd_buf[128] = {0};			
	int i = 0;
	upload_param_t *p_params = (upload_param_t *)arg;
	int file_type = (int)p_params->m_file_type;
	int file_num = (int)p_params->m_file_num;

	if (FILE_TYPE_IMG == file_type){
		if (p_params->m_p_file_path != NULL)
			sprintf((char*)cmd_buf, "screencap -p %s",  p_params->m_p_file_path);
		else
			sprintf((char*)cmd_buf, "screencap -p %s",  SCREEN_FILE);
	}

	while(file_num --){
		usleep(5000*1000);	
		if (strlen(cmd_buf)){
			system(cmd_buf);
		}
		if (FILE_TYPE_IMG == file_type){
			if (p_params->m_p_file_path){
				if (access(p_params->m_p_file_path, F_OK)!= -1){
					char new_name[128] = {0};
					sprintf((char*)new_name, "%s_%s_%d.png",  p_params->m_p_file_path, getCurrentTimeStr(), i++);
					rename(p_params->m_p_file_path, new_name);
					upload(FILE_UPLOAD_URL, new_name, "img", false);
				}
			}else{
				if (access(SCREEN_FILE, F_OK)!= -1){
					upload(FILE_UPLOAD_URL, SCREEN_FILE, "img", false);
				}
			}
		}else{
			upload(FILE_UPLOAD_URL, p_params->m_p_file_path, "upload", false);
		}
	}
	if (p_params->m_p_file_path){
		free(p_params->m_p_file_path);
	}

	if (p_params){
		free(p_params);
	}

	if (ws != NULL && ws->getReadyState() != WebSocket::CLOSED){
		ws->send("Upload done!");
	}

	return NULL;	
}

int upload_file_task(upload_param_t *p_params)
{
	pthread_t task_id;
	int ret = 0;
	pthread_attr_t thread_attr;
	int priority;

	ret = pthread_attr_init(&thread_attr);
	if (ret != 0){
		return -1;
	}

	ret = pthread_create(&task_id, &thread_attr, fun_upload_process, p_params);
	if (ret != 0){
		pthread_attr_destroy(&thread_attr);
		return -1;	
	}
	return 0;
}
