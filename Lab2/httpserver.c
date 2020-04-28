//启动http服务器命令：  ./fhttpd 端口号
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <pthread.h>
#include <getopt.h>

#define MAX_CLIENT_COUNT 10
#define RECV_BUF_SIZE 512
#define SEND_BUF_SIZE 512
int port = 0;

// 浏览器的请求类型
enum RequestType{REQUEST_GET, REQUEST_POST, REQUEST_UNDEFINED};

// 套接字应用 SO_REUSEADDR 套接字选项，以便端口可以马上重用
//SO_REUSEADDR是让端口释放后立即就可以被再次使用
void reuseAddr(int socketFd){
    int on = 1;
    int ret = setsockopt(socketFd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
    if(ret==-1){
        fprintf(stderr, "Error : fail to setsockopt\n");
        exit(1);
    }
}

//在本机启动tcp服务
int startTcpServer(char *ipAddr, int portNum){
    // 创建套接字 socket
    int httpdSocket = socket(AF_INET,SOCK_STREAM,0);
    if(httpdSocket==-1){
        fprintf(stderr,"Error: can't create socket,errno is %d\n",errno);
        exit(1);
    }

    //绑定端口 bind
    struct sockaddr_in tcpServerSockAddr;       //该结构体用来处理网络通信的地址
    memset(&tcpServerSockAddr, 0, sizeof(tcpServerSockAddr));//初始化
    tcpServerSockAddr.sin_family = AF_INET;
    tcpServerSockAddr.sin_port = htons(portNum);        //设置端口  htons()作用是将端口号由主机字节序转换为网络字节序的整数值
    //地址0.0.0.0 表示本机
    tcpServerSockAddr.sin_addr.s_addr = inet_addr(ipAddr);
    reuseAddr(httpdSocket);

    if(bind(httpdSocket,(const struct sockaddr*)&tcpServerSockAddr,     //强制类型转换 sockaddr_in 转换为 sockaddr
        sizeof(tcpServerSockAddr))==-1){
        fprintf(stderr, "Error: can't bind port %d,errno is %d\n",portNum,errno);
        exit(1);
    }

    //侦听 listen
    if(listen(httpdSocket,MAX_CLIENT_COUNT)==-1){           //最大客户数量
        fprintf(stderr,"Error: can't listen port %d,error is %d\n",portNum,errno);
        exit(1);
    }

    return httpdSocket;
}

// 功能: 从网络套接字socketFd读取一行到buf中，buf大小为bufLength,每行以\r\n结尾
// 返回: 读取的一行的字节数
int getOneLineFromSocket(int socketFd,char* buf,int bufLength){
    int byteCount = 0;
    char tmpChar;
    memset(buf, 0, bufLength);
    while(read(socketFd,&tmpChar,1) && byteCount<bufLength){
        if(tmpChar=='\r'){
            if(recv(socketFd,&tmpChar,1,MSG_PEEK)==-1){
                fprintf(stderr, "Error: fail to recv char after \\r\n");
                exit(1);
            }
            //如果 \r后面紧跟\n，表示一行结束
            if(tmpChar=='\n' && byteCount<bufLength){
                read(socketFd,&tmpChar,1);
                buf[byteCount++] = '\n';
                break;
            }
            buf[byteCount++] = '\r';
        }else{
            buf[byteCount++] = tmpChar;
        }
    }
    return byteCount;
}

//将指定字符串通过套接字发送出去
ssize_t socketSendMsg(int socketFd, const char* msg){
    return write(socketFd, msg, strlen(msg));
}

//向客户端发送文件
void responseStaticFile(int socketFd, int returnNum, char* filePath,
    char* contentType)
{
    char sendBuf[SEND_BUF_SIZE] = {0};

    if(strcmp(filePath,"./")==0){
        // 没有指定访问文件的，直接访问index.html
        filePath = "./index.html";
    }

    if(contentType==NULL){
        int tpFilePath = strlen(filePath)-1;
        while(tpFilePath>0){
            if(filePath[tpFilePath]!='.'){
                --tpFilePath;
            }else{
                break;
            }
        }
        if(tpFilePath){
            if(strcmp(filePath+tpFilePath+1,"html")==0){
                contentType = "text/html";
            }else if(strcmp(filePath+tpFilePath+1,"txt")==0){
                contentType = "text/plain";
            }else if(strcmp(filePath+tpFilePath+1,"png")==0){
                contentType = "image/png";
            }else if(strcmp(filePath+tpFilePath+1,"gif")==0){
                contentType = "image/gif";
            }else if(strcmp(filePath+tpFilePath+1,"jpeg")==0){
                contentType = "image/jpeg";
            }else if(strcmp(filePath+tpFilePath+1,"bmp")==0){
                contentType = "image/bmp";
            }else if(strcmp(filePath+tpFilePath+1,"webp")==0){
                contentType = "image/webp";
            }else if(strcmp(filePath+tpFilePath+1,"pdf")==0){
                contentType = "application/pdf";
            }
        }
    }
    
    FILE* pFile = fopen(filePath, "r");

    printf("%d %p : %s\n",returnNum,pFile,filePath);

    if(pFile==NULL){
        returnNum = 404;
        filePath = "./404.html";
        contentType = "text/html";
        pFile = fopen(filePath,"r");
    }
    switch(returnNum){
        case 200:
            sprintf(sendBuf,"HTTP/1.1 200 OK\r\n");
            break;
        case 404:
            sprintf(sendBuf,"HTTP/1.1 404 NOT FOUND\r\n");
            break;
        case 501:
            sprintf(sendBuf,"HTTP/1.1 501 Not Implemented\r\n");
            break;
        default:
            sprintf(sendBuf,"HTTP/1.1 %d Undefined Return Number\r\n",returnNum);
            break;
    }
    socketSendMsg(socketFd, sendBuf);

    sprintf(sendBuf, "Server: %s\r\n", "BeiAnPaiLe's web Server");
    socketSendMsg(socketFd, sendBuf);
    sprintf(sendBuf, "Content-type: %s\r\n", contentType);
    socketSendMsg(socketFd, sendBuf);
    sprintf(sendBuf, "Host: %s", "127.0.0.1:");
    char pp[10];
    sprintf(pp, "%d\r\n", port);
    strcat(sendBuf,pp);
    socketSendMsg(socketFd, sendBuf);
    socketSendMsg(socketFd, "\r\n");
    
    //向浏览器发送文件
    int readDataLen = 0;
    while((readDataLen=fread(sendBuf, 1, SEND_BUF_SIZE, pFile))!=0){
        write(socketFd, sendBuf, readDataLen);
        sendBuf[readDataLen] = 0;
    }

    fclose(pFile);
}

//响应POST并回显
void responseP(int socketFd, char* contentType, char* namevalue, char* idvalue)
{
    char sendBuf[SEND_BUF_SIZE] = {0};

    sprintf(sendBuf,"HTTP/1.1 200 OK\r\n");
    socketSendMsg(socketFd, sendBuf);

    sprintf(sendBuf, "Server: %s\r\n", "BeiAnPaiLe's web Server");
    socketSendMsg(socketFd, sendBuf);
    contentType = "text/html";
    sprintf(sendBuf, "Content-type: %s\r\n", contentType);
    socketSendMsg(socketFd, sendBuf);
    sprintf(sendBuf, "Host: %s", "127.0.0.1:");
    char pp[10];
    sprintf(pp, "%d\r\n", port);
    strcat(sendBuf,pp);
    socketSendMsg(socketFd, sendBuf);
    socketSendMsg(socketFd, "\r\n");

    sprintf(sendBuf, "<html><title>POST method</title><body>\r\n<style='text-align:center'>\r\nYour name:");
    strcat(sendBuf,namevalue);
    strcat(sendBuf,"\r\nID:");
    strcat(sendBuf,idvalue);
    strcat(sendBuf,"\r\n<hr><em>Http Web Server</em>\r\n</body></html>\r\n");
    socketSendMsg(socketFd, sendBuf);

}


//线程: 接收浏览器端的数据,并返回一个http包
void* responseBrowserRequest(void* ptr){
    int browserSocket = *((int*)ptr);
    free(ptr);
    char c;
    char recvBuf[RECV_BUF_SIZE+1] = {0};
    int contentLength = 0;
    enum RequestType requestType = REQUEST_UNDEFINED;
    
    int cur_cpu = sched_getcpu();
    printf("current running cpu:%d\n", cur_cpu);
    
    // 定义存放请求文件名的内存块
    #define FILE_PATH_LENGTH 128
    char requestFilePath[FILE_PATH_LENGTH] = {0};

    // 定义存放查询参数字符串的内存库
    #define QUERY_STRING_LENGTH 128
    char PostString[QUERY_STRING_LENGTH] = {0};

    char name[5] = {0};
    char namevalue[QUERY_STRING_LENGTH] = {0};
    char id[3] = {0};
    char idvalue[QUERY_STRING_LENGTH] = {0};

    // 将 http数据包的信息头读完(信息头和正文间以空行分隔)
    while(getOneLineFromSocket(browserSocket,recvBuf,RECV_BUF_SIZE)){   //请求行+首部行
        // printf("%s",recvBuf);
        if(strcmp(recvBuf, "\n")==0){
            break;
        }

        if(requestType==REQUEST_UNDEFINED){         //第一行会进入此判断 
            int pFileName = 0;
            int pQueryString = 0;
            int pRecvBuf = 0;

            if(strncmp(recvBuf,"GET",3)==0){        //请求行的方法部分  这里包含了小写拒绝
                // GET 请求
                requestType = REQUEST_GET;
                pRecvBuf = 4;
                
            }else if(strncmp(recvBuf,"POST",4)==0){
                // POST 请求
                requestType = REQUEST_POST;
                pRecvBuf = 5;
            }

            // 获取请求的文件路径 查询参数
            if(pRecvBuf){       //即请求行的URL部分
                requestFilePath[pFileName++] = '.';
                while(pFileName<FILE_PATH_LENGTH && recvBuf[pRecvBuf]
                    && recvBuf[pRecvBuf]!=' ' && recvBuf[pRecvBuf]!='?'){
                    requestFilePath[pFileName++] = recvBuf[pRecvBuf++];
                }

                if(pFileName<FILE_PATH_LENGTH && recvBuf[pRecvBuf]=='?'){       //读取GET方法下 URL最后‘？’后面的信息
                    ++pRecvBuf;
                    while(pQueryString<QUERY_STRING_LENGTH && 
                        recvBuf[pRecvBuf] && recvBuf[pRecvBuf]!=' '){
                        PostString[pQueryString++] = recvBuf[pRecvBuf++];
                    }
                }
            }
        }else if(requestType==REQUEST_GET){

        }else if(requestType==REQUEST_POST){
            if(strncmp(recvBuf, "Content-Length:", 15)==0){         //在contentlength行进入此判断  获取请求体长度（两个字符串的前15个字符要相同）
                contentLength = atoi(recvBuf+15);
            }
        }
    }

    //如果是REQUEST_GET或REQUEST_UNDEFINED类型，不再读取http正文的内容
    //如果是REQUEST_POST类型,在读取contentLength长度的数据
    if(requestType==REQUEST_POST && contentLength){
        if(contentLength > QUERY_STRING_LENGTH){
            fprintf(stderr, "Query string buffer is smaller than content length\n");
            contentLength = QUERY_STRING_LENGTH;
        }
        read(browserSocket, PostString, contentLength);
    }

    int i=0, t=0, k=0;
    for(i=0;i<4;i++) {name[i]=PostString[t]; t++;}
    if(strcmp(name,"Name")==0){
        i=0; t++;
        while(PostString[t]!='&'){namevalue[i]=PostString[t]; t++; i++;}
        t++;
        for(i=0;i<2;i++) {id[i]=PostString[t]; t++;}
        if(strcmp(id,"ID")==0){
            k=1; t++;
            for(i=0;i<contentLength;i++) {idvalue[i]=PostString[t]; t++;}
        }
    }
    
    // 判断请求的文件是否是文件夹
    struct stat fileInfo;
    stat(requestFilePath,&fileInfo);
    if(S_ISDIR(fileInfo.st_mode)){
        //是文件夹的情况
        if(strcmp(requestFilePath,"./Post_show")!=0) k=0;
        char tt[11]="/index.html";
        strcat(requestFilePath,tt);
    }


    switch(requestType){
        case REQUEST_GET:
                responseStaticFile(browserSocket,200,requestFilePath,NULL);
            break;
        case REQUEST_POST:
            {
                if(contentLength==0){
                    responseStaticFile(browserSocket,404,"./404.html","text/html");
                    break;
                }
                else if(k==0){
                    responseStaticFile(browserSocket,404,"./404.html","text/html");
                }
                else{
                    responseP(browserSocket,"text/html",namevalue,idvalue);
                }
            }
            break;
        case REQUEST_UNDEFINED:
            {
                responseStaticFile(browserSocket, 501, "./501.html","text/html");
            }
            break;
        default:
            break;
    }
    
    close(browserSocket);
    return NULL;
}

int main(int argc,char* argv[]){
    //长选项
    char *string = "a:b:c:d";
    static struct option long_options[] =
    {  
        {"ip", required_argument, NULL, '1'},
        {"port", required_argument, NULL, '1'},
        {"number-thread", required_argument, NULL,'1'},
        {NULL,  0,   NULL, 0},
    }; 
    //判断命令是否正确
    if(argc < 7){
        fprintf(stderr,"USAGE: %s --ip IPaddr --port portNum --number-thread threadNum\n",argv[0]);
        exit(1);
    }
    int portNum = atoi(argv[4]);
    port = portNum;
    char *ipAddr=argv[2];

    //限定开启http服务的端口号为1024~65535或者是80
    if((portNum!=80)&&(portNum<1024 || portNum>65535)){
        fprintf(stderr,"Error: portNum range is 1024~65535 or 80\n");
        exit(1);
    }

    int httpdSocket = startTcpServer(ipAddr, portNum);

    while(1){
        struct sockaddr_in browserSocketAddr;
	int* browserSocket = (int *)malloc(sizeof(int));
	int browserLen = sizeof(browserSocketAddr);

        *browserSocket = accept(httpdSocket,
            (struct sockaddr*)&browserSocketAddr,&browserLen);
        if(*browserSocket < 0){
            fprintf(stderr,"Error: fail to accept, error is %d\n",errno);
            exit(1);
        }
	//输出请求方地址及端口
        printf("%s:%d linked !\n",inet_ntoa(browserSocketAddr.sin_addr),
            browserSocketAddr.sin_port);        
        pthread_t responseThread;
        // 创建线程处理浏览器请求
        int threadReturn = pthread_create(&responseThread,
            NULL,responseBrowserRequest,(void *)browserSocket);
	// 
	pthread_detach(responseThread);
        // 如果pthread_create返回不为0,表示发生错误
        if(threadReturn){
            fprintf(stderr,"Error: fail to create thread, error is %d\n",threadReturn);
            exit(1);
        }
    }

    return 0;
}
