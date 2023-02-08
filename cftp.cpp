#include "cftp.h"
#include "definitions.h"

const int cFTP::DEFAULT_PORT = INTERNET_SERVICE_FTP;
const int cFTP::MODE_BINARY = FTP_TRANSFER_TYPE_UNKNOWN;
const int cFTP::MODE_ASCII = FTP_TRANSFER_TYPE_ASCII;
const int cFTP::MODE_UNKNOWN = FTP_TRANSFER_TYPE_UNKNOWN;

cFTP::cFTP()
{
    //ctor
}

cFTP::~cFTP()
{
    this->close();
    //dtor
}

bool cFTP::Connect() {
    this->hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
    if (this->hInternet == NULL) {
        std::cout << "InternetOpen() failed" << std::endl;
        return false;
    }

    this->hFtpSession = InternetConnect(this->hInternet, this->host.c_str(), this->port, this->username.c_str(), this->password.c_str(), INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
    if (hFtpSession == NULL) {
        std::cout << "InternetConnect() failed" << std::endl;
        return false;
    }
    return true;
}

bool cFTP::Reconnect() {
    this->close();
    this->Connect();
}

void cFTP::close() {
    InternetCloseHandle(this->hFtpSession);
    InternetCloseHandle(this->hInternet);
}

void cFTP::setHost(std::string host) {
    this->host = host;
}

void cFTP::setUsername(std::string user) {
    this->username = user;
}

void cFTP::setPassword(std::string pass) {
    this->password = pass;
}

void cFTP::setPort(int port) {
    this->port = port;
}

bool cFTP::sendFile(std::string localFile, std::string remoteFile, int mode) {
    return FtpPutFile(this->hFtpSession, localFile.c_str(), remoteFile.c_str(), mode, 0);
}

/*
u_int64 cFTP::getFileSizeOld(std::string fileName) {
    DWORD dwRet = -1;
    DWORD dwFileSize = 0;
    HINTERNET hFtpFile = NULL;
    char* CommandResult;
    FtpCommand(this->hFtpSession, FALSE, FTP_TRANSFER_TYPE_ASCII, "SIZE", 0, (void**)CommandResult);
    // Open the file on the FTP server
    hFtpFile = FtpOpenFile(this->hFtpSession, fileName.c_str(), GENERIC_READ, FTP_TRANSFER_TYPE_UNKNOWN, 0);
    if (hFtpFile == NULL)
    {
        std::cout << GetLastError() << std::endl;
        throw std::runtime_error("FtpOpenFile error");
    }

    // Get the file size
    if (!FtpGetFileSize(hFtpFile, &dwFileSize))
    {
        std::cout << GetLastError() << std::endl;
        throw std::runtime_error("FtpGetFileSize error");
        // return GetLastError();
    }

    return dwFileSize;
}
*/

#include <Winsock2.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t cFTP::getFileSize(std::string fileName)
{
    int sockfd;
    struct sockaddr_in sa;
    std::string buffer;
    int byteCount;

    //create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error creating socket");
        exit(1);
    }

    //setup address structure
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(this->host.c_str());
    sa.sin_port = htons(this->port);

    //connect to ftp server
    if (connect(sockfd, (struct sockaddr*)&sa, sizeof(sa)) < 0)
    {
        perror("Error connecting");
        exit(1);
    }

    buffer = recv_block_until_cr(sockfd);

    //authenticate with ftp user and password;
    buffer = "USER "+this->username+"\r\n";
    if (send(sockfd, buffer.c_str(), buffer.size(), 0) < 0)
    {
        perror("Error sending");
        exit(1);
    }
    // byteCount = recv(sockfd, buffer, 1024, 1);
    // buffer[byteCount] = 0;
    // printf("%s\n", buffer);
    buffer = recv_block_until_cr(sockfd);


    // sprintf(buffer, "PASS %s\r\n", this->password.c_str());
    buffer = "PASS "+this->password+"\r\n";
    if (send(sockfd, buffer.c_str(), buffer.size(), 0) < 0)
    {
        perror("Error sending");
        exit(1);
    }
    buffer = recv_block_until_cr(sockfd);
    // printf("%s\n", buffer);

    //request the file size
    // sprintf(buffer, "SIZE %s\r\n", fileName.c_str());
    buffer = "SIZE "+fileName+"\r\n";
    if (send(sockfd, buffer.c_str(), buffer.size(), 0) < 0)
    {
        perror("Error sending");
        exit(1);
    }
    // byteCount = recv(sockfd, buffer, 1024, 1);
    // buffer[byteCount] = 0;
    buffer = recv_block_until_cr(sockfd);

    std::vector<std::string> strList = split(buffer);
    if(strList[0]=="213") {
        return std::stoi(strList[1]);
    } else {
        return -1;
    }
}

// Function that uses poll to block until a carriage return (CR) character
// is read from the given socket fd.
std::string cFTP::recv_block_until_cr(int sock) {
    // Function to receive a line (until CR LF) over socket
    std::string response = "";
    char buffer[1] = {0};
    while( recv(sock, buffer, 1, 0) > 0 )
    {
        if( buffer[0] == '\r' )
        {
            // Check the next byte
            recv(sock, buffer, 1, MSG_PEEK);
            // If it is LF, then read it from the socket
            if( buffer[0] == '\n' ) recv(sock, buffer, 1, 0);
            break;
        }
        response += buffer[0];
    }
    return response;
}

std::vector<std::string> split(std::string s){
    std::vector<std::string> strings;
    std::string word;
    for (int i = 0; i < s.length(); i++){
        if (s[i] == ' ') {
            strings.push_back(word);
            word = "";
        }
        else {
            word += s[i];
        }
    }
    strings.push_back(word);
    return strings;
}
