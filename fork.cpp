#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h> 
#include <errno.h> 
#include <string>
#include <stdexcept>
#include <iostream>
#include <sys/ioctl.h>
#define SERVER_PORT 8888 
#define BUFFER_LENGTH 1024
using namespace std;
class SocketException : public std::logic_error
{
public:
SocketException(const std::string& what) throw ()
: std::logic_error(what) {};
};
int SocketRead(int& fd, std::string& strValue)
{
// 每次只读取一个字节
int nRead = 1;
auto_ptr<char> apBuf(new char[nRead]);
int r = read(fd, apBuf.get(), nRead) ;
if (0 < r)
{
   strValue.assign(apBuf.get(), r);
}
return r;
}
int SocketWrite(int& fd, const char* pBuf, int nBufLen)
{
int nWrite = 0;
int nError = -1;
/*
SIGPIPE：sent to a process when it attempts to write to a pipe without a process connected to the other end. The symbolic constant for SIGPIPE is defined in the header file signal.h. Symbolic signal names are used because signal numbers can vary across platforms.
*/
// SIG_IGN:忽略信号
sighandler_t sh = signal(SIGPIPE, SIG_IGN);
while (nWrite < nBufLen)
{
   nError = send(fd, pBuf + nWrite, nBufLen - nWrite, 0); 
   if (0 > nError)
   {
    perror(strerror(errno));
    // 见文末
    if (nError == EINTR) continue;
    if (nError == EPIPE) 
    {
     close(fd); 
    }
    // 恢复信号
    signal(SIGPIPE, sh);
    throw SocketException(string("SocketWrite error, ") + strerror(errno));
   }
   else if (0 == nError)
   {
    signal(SIGPIPE, sh);
    throw SocketException(string("SocketWrite error, ") + strerror(errno));
   }
   else
   {
    nWrite += nError;
   }
}
signal(SIGPIPE, sh);
return nError;
}
void SetOption(int s, int level, int optname, const void *optval, socklen_t optlen)
{
if (setsockopt(s, level, optname, optval, optlen) < 0)
{
   perror(strerror(errno));
}
}
int main(int argc, char*argv[])
{
int fdServer = -1;
int   fdClient = -1;
int   nStatus = -1;
pid_t pid = 0;
int nSockAddrLen = sizeof(struct sockaddr_in);
struct sockaddr_in addrServer;
struct sockaddr_in addrSocket;
bzero(&addrServer, sizeof(struct sockaddr_in)); 
bzero(&addrSocket, sizeof(struct sockaddr_in));
if(-1 == (fdServer = socket(AF_INET, SOCK_STREAM, 0)))
{
   perror(strerror(errno));
   exit(-1);
}
int nReuseAddr = 1;
SetOption(fdServer, SOL_SOCKET, SO_REUSEADDR, &nReuseAddr, sizeof(int));
addrServer.sin_family = AF_INET;
addrServer.sin_port = htons(SERVER_PORT);
addrServer.sin_addr.s_addr = INADDR_ANY;
if (-1 == bind(fdServer, (struct sockaddr *)&addrServer, sizeof(struct sockaddr))) 
{ 
   perror(strerror(errno));
   exit(-1);
}
if (-1 == listen(fdServer, 128))
{
   perror(strerror(errno));
   exit(-1);
}

while (true)
{
   if ((fdClient = accept(fdServer, (struct sockaddr *)&addrSocket, (socklen_t*)&nSockAddrLen)) < 0)
   {
    perror(strerror(errno));
    exit(-1);
   }
   printf("client address:%s\t port:%d\r\n", inet_ntoa(addrSocket.sin_addr), ntohs(addrSocket.sin_port));
   if ((pid = fork()) < 0)
   {
    perror(strerror(errno));
    exit(-1);
   }
   else if (0 == pid) /* clild */
   {
    while (true)
    {
     try
     {
      std::string strValue;
      if(0 < SocketRead(fdClient, strValue))
      {
       SocketWrite(fdClient, strValue.c_str(), strValue.length());
      }
      else
      {
       close(fdClient); 
      }
      // 要有延时，不然CPU使用率很高
      usleep(10);
     }
     catch(const SocketException& e)
     {
      cerr << e.what() << endl; 
      close(fdClient);
      break;
     }
    }
   }
   else /* parent */
   {
    close(fdClient); 
   }
}
return 0;
}
