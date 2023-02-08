#ifndef CFTP_H
#define CFTP_H

#include <iostream>
#include <windows.h>
#include <WinInet.h> // add "wininet" library to link wininet.h api functions
#include <ctype.h>
#include <cstdint>
#include <stdexcept>
#include <vector>

std::vector<std::string> split(std::string str);

class cFTP
{
    public:
        static const int DEFAULT_PORT;
        static const int MODE_BINARY;
        static const int MODE_ASCII;
        static const int MODE_UNKNOWN;


        cFTP();
        virtual ~cFTP();
        bool Connect();
        void close();
        void setHost(std::string host);
        void setUsername(std::string user);
        void setPassword(std::string pass);
        void setPort(int port);
        bool sendFile(std::string localFile, std::string remotelFile, int mode = FTP_TRANSFER_TYPE_ASCII);
        uint64_t getFileSize(std::string fileName);
        std::string recv_block_until_cr(int fd);

    protected:

    public:


        HINTERNET hInternet;
        HINTERNET hFtpSession;
        std::string host, username, password;
        int         port = INTERNET_SERVICE_FTP;
};

#endif // CFTP_H
