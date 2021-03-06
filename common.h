#ifndef _COMMON_H_
#define _COMMON_H_

//#define WS_PROTOCOL "ws://"
//#define HTTP_PROTOCAL "http://"
//#define DOMAIN "fun.ws.test"
//#define PORT_NUM ":8000"
//#define STREAM_PATH "/chat/stream"
//#define CONNECT_PATH "/chat/connect"
//
//#define WS_URL WS_PROTOCOL DOMAIN PORT_NUM
//#define HTTP_URL HTTP_PROTOCAL DOMAIN PORT_NUM
//
//#define WS_URL_CONNECT WS_URL CONNECT_PATH
//#define FILE_UPLOAD_URL HTTP_URL "/chat/client_upload"
//#define WS_STREAM_URL WS_URL STREAM_PATH
//
//#define WS_STREAM_URL "ws://59.172.252.67:8100/chat/stream"
//#define WS_URL "ws://59.172.252.67:8100/chat/connect"
//#define FILE_UPLOAD_URL "http://59.172.252.67:8100/chat/client_upload"

//#define WS_STREAM_URL "ws://172.17.12.189:8000/chat/stream"
//#define WS_URL_CONNECT "ws://172.17.12.189:8000/chat/connect"
//#define FILE_UPLOAD_URL "http://172.17.12.189:8000/chat/client_upload"



#define WS_STREAM_URL "ws://172.17.7.11:8000/chat/stream"
#define WS_URL_CONNECT "ws://172.17.7.11:8000/chat/connect"
#define FILE_UPLOAD_URL "http://172.17.7.11:8000/chat/client_upload"



#define TMP_FILE "/tmp/wsdata"
#define FIFO "/tmp/stdout_fifo"

#define SCREEN_FILE "/tmp/screen.png"
#define SCREEN_SPEED_TEST_FILE "/tmp/speed"
#define SCREEN_LINK_TEST_FILE "/tmp/link"

#define SPEED_TEST_CMD "SpeedTest"
#define LINK_TEST_CMD "LinkTest"
#define GET_FILE_CMD "pull"
#define EXIT_CMD "exit"


#define BUF_SIZE_MAX (4*1024)
#define BUF_SIZE_LINE 512

#define SPEED_TEST_ACTIVITY_CMD "am start -n tv.fun.master/.ui.activity.NetworkTestActivity"
#define LINK_TEST_ACTIVITY_CMD "am start -n tv.fun.master/.ui.activity.NetworkCheckActivity"


#endif
