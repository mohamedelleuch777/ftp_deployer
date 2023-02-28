#ifndef CFTP_SOCK_H
#define CFTP_SOCK_H


#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>
#ifdef WIN32
    #include <winsock2.h>
    #include <wininet.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif




using namespace std;


class cFTP_Sock
{
public:
    cFTP_Sock();
    virtual ~cFTP_Sock();
    bool Connect();
    string receiveResponse();
    bool login();
    int readResponse(string resps);
    bool sendFile(string fileName, string remotePath, int mode=FTP_TRANSFER_TYPE_ASCII);
    bool sendFile2(string localFile, string remoteDirPath, string fileName,int mode=FTP_TRANSFER_TYPE_ASCII);
    long getFileSize(string fileName);


    void setHost(std::string host);
    void setUsername(std::string user);
    void setPassword(std::string pass);
    void setPort(int port);
    bool ExecuteCommand(std::string cmd);


private:
    int     sock;

    string host, username, password;
    int         port = INTERNET_SERVICE_FTP;
};


vector<string> splitByComma(string s, char c);

#endif // CFTP_SOCK_H
