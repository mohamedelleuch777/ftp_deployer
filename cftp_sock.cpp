#include "cftp_sock.h"

cFTP_Sock::cFTP_Sock()
{
    #ifdef WIN32
        // Initialize Winsock
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            cerr << "WSAStartup failed: " << result << endl;
            return;
        }
    #endif
}

cFTP_Sock::~cFTP_Sock() {
    #ifdef WIN32
        // Clean up Winsock
        WSACleanup();
    #endif
}

bool cFTP_Sock::Connect() {
    this->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error creating socket" << endl;
        return false;
    }


    // Get the IP address of the FTP server
    struct hostent* hostEnt = gethostbyname(this->host.c_str());
    if (hostEnt == nullptr) {
      cerr << "gethostbyname failed: " << WSAGetLastError() << endl;
      closesocket(sock);
      WSACleanup();
      return false;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(this->port);
    address.sin_addr = *((struct in_addr*)hostEnt->h_addr);

    int result = connect(sock, (struct sockaddr*)&address, sizeof(address));
    if (result == SOCKET_ERROR) {
        cerr << "connect failed: " << WSAGetLastError() << endl;
        closesocket(sock);
        WSACleanup();
        return false;
    }


    string res;

    // Wait for the FTP server's response
    res = this->receiveResponse();
    if(this->readResponse(res)!=220) {
        cerr << "Error not connected" << endl;
        return false;
    }

    cout << "CONNECTED TO SERVER...AS ANONYMOUS" << endl;
    cout << "Welcome message received:" << endl;
    cout << res.substr(4,res.size()) << endl;
    return true;
}

string cFTP_Sock::receiveResponse() {
  const int BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];
  string response;

  int bytesReceived = 0;
  while ((bytesReceived = recv(this->sock, buffer, BUFFER_SIZE, 0)) > 0) {
    response += string(buffer, bytesReceived);
    if (response.find("\r\n") != string::npos) {
      break;
    }
  }

  if (bytesReceived == 0) {
    cerr << "Connection closed by server" << endl;
  } else if (bytesReceived < 0) {
    cerr << "Error receiving response" << endl;
  }

  return response;
}

bool cFTP_Sock::login() {

    string res, request;

    // send user account
    request = "USER " + this->username;
    this->ExecuteCommand(request);
    // Wait for the FTP server's response
    res = this->receiveResponse();
    if(this->readResponse(res)!=331) {
        cerr << "Error username is not ok" << endl;
        return false;
    }

    // send user password
    request = "PASS " + this->password;
    this->ExecuteCommand(request);
    // Wait for the FTP server's response
    res = this->receiveResponse();
    if(this->readResponse(res)!=230) {
        cerr << "Error password not ok" << endl;
        return false;
    }

    cout << "Successfully Logged in" << endl;
    return true;
}

void cFTP_Sock::setHost(std::string host) {
    this->host = host;
}

void cFTP_Sock::setUsername(std::string user) {
    this->username = user;
}

void cFTP_Sock::setPassword(std::string pass) {
    this->password = pass;
}

void cFTP_Sock::setPort(int port) {
    this->port = port;
}

int cFTP_Sock::readResponse(string resps) {
    string f = resps.substr(0,3);
    int r = std::stoi(f);
    return r;
}


bool cFTP_Sock::sendFile(string fileName, string remotePath, int mode)
{
    // Open the file for reading
    ifstream file(fileName, ios::binary | ios::ate);
    if (!file.is_open())
    {
        cout << "Failed to open the file." << endl;
        return false;
    }

    // Get the size of the file
    int fileSize = (int)file.tellg();
    file.seekg(0, ios::beg);

    // Allocate a buffer for the file contents
    char *buffer = new char[fileSize];

    // Read the file into the buffer
    file.read(buffer, fileSize);
    file.close();

    // Determine the transfer mode (ASCII or binary)
    string transferMode;
    if (mode == FTP_TRANSFER_TYPE_ASCII)
    {
        transferMode = "ASCII";
    }
    else if (mode == FTP_TRANSFER_TYPE_BINARY)
    {
        transferMode = "BINARY";
    }
    else
    {
        transferMode = "UNKNOWN";
    }
    transferMode += "\r\n";

    // Send the transfer mode to the FTP server
    send(this->sock, transferMode.c_str(), transferMode.length(), 0);

    string res = this->receiveResponse();

    // Send the remote path and file size to the FTP server
    string message = "STOR " + remotePath + " " + to_string(fileSize) + "\r\n";
    send(this->sock, message.c_str(), message.length(), 0);

    res = this->receiveResponse();

    int totalSent = 0;
    while (totalSent < fileSize)
    {
        int bytesSent = send(this->sock, buffer + totalSent, fileSize - totalSent, 0);
        if (bytesSent == SOCKET_ERROR)
        {
            cout << "Error sending data to the FTP server." << endl;
            delete[] buffer;
            return false;
        }

        totalSent += bytesSent;

        cout << "Sent " << (int)(((double)totalSent / fileSize) * 100) << "% of the file." << endl;
    }

    // Clean up the buffer
    delete[] buffer;

    return true;
}

long cFTP_Sock::getFileSize(string fileName) {

    string res;
    //request the file size
    // sprintf(buffer, "SIZE %s\r\n", fileName.c_str());
    string buffer = "SIZE "+fileName+"\r\n";
    if (send(this->sock, buffer.c_str(), buffer.size(), 0) < 0)
    {
        perror("Error sending");
        exit(1);
    }

    // Wait for the FTP server's response
    res = this->receiveResponse();
    if(this->readResponse(res)==550) {
        cerr << "Error file not found!" << endl;
        return 0;
    }
    if(this->readResponse(res)!=213) {
        cerr << "Error could not read the file size" << endl;
        return -1;
    }

    int p = res.find(" ",4);
    long r = stol(res.substr(4,p-4+1));
    return r;
}

bool cFTP_Sock::ExecuteCommand(std::string cmd) {
    std::string internalCommand = cmd;
    internalCommand.push_back('\r');
    internalCommand.push_back('\n');
    int res = send(this->sock, internalCommand.c_str(), internalCommand.length(), 0);
    if (res < 0) {
        cerr << "Error sending FTP command" << endl;
        return false;
    }
    return true;
}

bool cFTP_Sock::sendFile2(string localFile, string remoteDirPath, string fileName, int mode)
{
    //entering passive mode
    string command = "CWD "+remoteDirPath;
    this->ExecuteCommand(command);
    string res = this->receiveResponse();
    int resInt = this->readResponse(res);
    if(resInt!=250) {
        cout << "Failed to change directory to: " << remoteDirPath << endl;
        return false;
    }

    //set type to A
    command = "TYPE A";
    this->ExecuteCommand(command);
    res = this->receiveResponse();
    resInt = this->readResponse(res);
    if(resInt!=200) {
        cout << "Could not set type to A" << endl;
        return false;
    }


    //passing to passive mode
    command = "PASV";
    this->ExecuteCommand(command);
    res = this->receiveResponse();
    resInt = this->readResponse(res);

    if(resInt!=227) {
        cout << "Could not pass to passive mode" << endl;
        return false;
    }
    unsigned pos1 = res.find("(");
    unsigned pos2 = res.find(")");
    unsigned len  = pos2 - 1 - pos1;
    string addrPort = res.substr(pos1+1, len);

    vector<string> resV = splitByComma(addrPort, ',');
    int newPort = stoi(resV[4])*256 + stoi(resV[5]);

    // ######################################################
    // CREATE NEW SOCKET FOR THE DATA CHANNEL CONNECTION
    // ######################################################
    int newSock = socket(AF_INET, SOCK_STREAM, 0);
    if (newSock < 0) {
        cerr << "Error creating socket" << endl;
        return false;
    }

    // Get the IP address of the FTP server
    struct hostent* hostEnt = gethostbyname(this->host.c_str());
    if (hostEnt == nullptr) {
      cerr << "gethostbyname failed: " << WSAGetLastError() << endl;
      closesocket(newSock);
      WSACleanup();
      return false;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(newPort);
    address.sin_addr = *((struct in_addr*)hostEnt->h_addr);

    int result = connect(newSock, (struct sockaddr*)&address, sizeof(address));
    if (result == SOCKET_ERROR) {
        cerr << "connect data channel failed: " << WSAGetLastError() << endl;
        closesocket(newSock);
        WSACleanup();
        return false;
    }
    // ######################################################
    // END
    // ######################################################
/*
    // Determine the transfer mode (ASCII or binary)
    string transferMode;
    if (mode == FTP_TRANSFER_TYPE_ASCII)
        transferMode = "ascii";
    else if (mode == FTP_TRANSFER_TYPE_BINARY)
        transferMode = "binary";
    else
        transferMode = "unknown";

    // Send the transfer mode to the FTP server
    this->ExecuteCommand(transferMode);
    res = this->receiveResponse();
*/

    //Send command to upload the file
    command = "STOR " + fileName;
    this->ExecuteCommand(command);
    res = this->receiveResponse();


    //Open the file
    ifstream fileStream(localFile.c_str(), ios::binary);
    if (!fileStream.is_open())
    {
        return false;
    }

    //Send the file content
    while (!fileStream.eof())
    {
        char buffer[1024];
        fileStream.read(buffer, 1024);
        int readBytes = fileStream.gcount();
        if (readBytes > 0)
        {
            send(newSock, buffer, readBytes, 0);
            // this->ExecuteCommand("DATA\r\n"+string(buffer)+"\r\n.\r\n");
        }
    }
    fileStream.close();
    //Close the data channel connection
    closesocket(newSock);
    // WSACleanup();

    res = this->receiveResponse();
    resInt = this->readResponse(res);
    if(resInt!=226) {
        cout << "Error while transfering the file: " << localFile << endl;
        return false;
    }




    // Clean up the buffer
    // delete[] buffer;

    return true;
}

// Function to split a string by comma and store the list of strings in a vector of string
vector<string> splitByComma(string s, char c)
{
    // Vector to store the strings
    vector<string> result;

    // To store the intermediate strings
    string str;

    // Traverse through the input string
    for (auto x : s) {

        // If x is a comma, then store the intermediate string
        if (x == c) {
            result.push_back(str);
            str = "";
        }

        // Else keep adding characters to form the intermediate string
        else {
            str = str + x;
        }
    }

    // Store the last string
    result.push_back(str);

    // Return the vector
    return result;
}
