#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>
#include "cftp.h"
#include "cftp_sock.h"
#include "settings.h"
#include <dirent.h>
#include "definitions.h"
#include <ctime>

using namespace std;

int requestFileSize(string fileName);
int getLocalFileSize(string fileName);
std::vector<std::string> listAllJsFiles(std::string path, string skipThisOne);
void getFileName(string path, string* name, string *extension);
std::string CopyFileWithMin(std::string sourcePath, std::string destinationPath);
void eventLog(std::string message);
void printProgressBar(double percentage);
string ExePath();
void exec(string program, string param);
//void printLastLine(std::string text);

cFTP ftp;
cFTP_Sock ftpSock;
int attempt = 0;
int Max_Attempt;
int lastExecutionValue = 0;

int main(int argc, char **argv)
{
    if (argc >= 2) {
         string param(argv[1]);
         lastExecutionValue = stoi(param);
    }
    Settings st;
    srand(time(0));  // Initialize random number generator.

    char charList[] = {'-', '\\', '|', '/'};
    int max = 0;
    if(lastExecutionValue==0) {
        cout << "Preparing Network For The Deploy..." << endl;
        max = 7 + rand() % 10;

    }
    else {
        cout << "Checking last deploy operation..." << endl;
        max = 1 + rand() % 10;
    }
    for(int r=0;r<max; r++) {
        cout << charList[r%3] << '\r';
        Sleep(250);
        cout.flush();
    }

    Max_Attempt = st.maxAttempt;
    ftpSock.setHost(st.host);
    ftpSock.setUsername(st.username);
    ftpSock.setPassword(st.password);
    ftpSock.setPort(st.port);
    if(!ftpSock.Connect()) {
        cout << endl << "FTP CONNECT FAILED" << endl;
        system("pause");
        return 5;
    }
    if(!ftpSock.login()) {
        cout << "FTP LOGIN FAILED" << endl;
        system("pause");
        return 8;
    }


    cout << "CONNECTED TO SERVER...\nSTARTING DEPLOY" << endl;
    // Declare variables
    vector<string> fileList;
    std::string remoteFolder = st.remoteDirectory;
    std::string localFolder = st.localDirectory;
    string fileName, remoteFileName, locae, localFileName;
    long localFileSize, remoteFileSize; // Size;

    // Get the file list of the directory
    fileList = listAllJsFiles(localFolder, st.minifiedPath);


    eventLog("####################################Deploy Started!!!!!####################################");

    // Upload files
    for (unsigned i = 0; i < fileList.size(); i++)
    {
        // Get file names
        fileName = fileList[i];

        //Find the position of the substring
        int pos = fileName.find(localFolder);
        //Calculate the length of the substring
        int len = localFolder.size() - pos; // + 1;
        //Remove the substring
        fileName.erase(pos, len);

        localFileName = localFolder + fileName;
        std::string nm, ex;
        getFileName(fileName, &nm, &ex);
        remoteFileName = remoteFolder + "/" + nm + ".min." + ex;

        string minifiedFilePath = "";

        // Prepare file to be copied
        if(st.minify) {
            // Minify the file
        } else {
            // Copy it to the minified path with adding .min to the name
            string minPath = st.minifiedPath;
            minifiedFilePath = CopyFileWithMin(localFileName, minPath);
        }

        // Draw a progress bar
        printProgressBar(((double)i+1)*100/fileList.size());

        // Get file sizes
        localFileSize = getLocalFileSize(minifiedFilePath);
        if(localFileSize == 0) {
            continue; // skip upload
        }
        // remoteFileSize = ftp.getFileSize(remoteFileName);
        remoteFileSize = ftpSock.getFileSize(remoteFileName);
        // Check if file sizes match
        if (localFileSize == remoteFileSize) {
            continue; // skip upload
        }

        // Retry uploading file until file sizes match
        bool uploadComplete = false;

            cout << endl << "Trying to send: " << fileName << endl;
        while (!uploadComplete)
        {
            // Upload file
            // bool uploadResult = ftp.sendFile(minifiedFilePath.c_str(), remoteFileName.c_str(), cFTP::MODE_ASCII);
            bool uploadResult = ftpSock.sendFile2(minifiedFilePath, st.remoteDirectory,  nm + ".min." + ex, cFTP::MODE_ASCII);
            if (!uploadResult) {
                lastExecutionValue = 0;
                unsigned errCode = GetLastError();
                std::string err = "Send File failed: " + std::to_string(errCode);
                if(attempt>Max_Attempt) {
                    cout << "Deploy Failed to be Completed !!!" << endl;
                    eventLog("#############################Deploy Failed to be Completed !!!#############################");
                    system("pause");
                    throw std::runtime_error(err);
                    return errCode;
                }
                else {
                    cout << "error code: " << errCode << endl;
                    cout << err << endl;
                    cout << "Retry to send the same file again. Attempt: " << attempt << endl;
                    eventLog("Failed Error: " + std::to_string(errCode) + ". Retry " + std::to_string(attempt) + ": " + fileName);
                    attempt++;
                    if(errCode==12030 || errCode==12111 || errCode==10054 || errCode==10093) {
                        bool result = ftpSock.Reconnect();
                        if(result) {
                            // Draw a progress bar
                            printProgressBar(((double)i+1)*100/fileList.size());
                        }
                        else {
                            cout << "Reconnected was failed!!!" << endl;
                        }
                    }
                    continue;
                }
            }

            // Reset the attempts counter
            attempt = 0;
            // Get file sizes
            localFileSize = getLocalFileSize(minifiedFilePath);
            remoteFileSize = ftpSock.getFileSize(remoteFileName);
            // remoteFileSize = requestFileSize(remoteFileName);

            // Check if file sizes match
            if (localFileSize == remoteFileSize) {
                uploadComplete = true;
                // Print success message
                cout << "Successfully uploaded: " << fileName << endl;
                eventLog("Successfully uploaded: " + fileName);
            }
            else {
                cout << "File was broken: " << fileName << endl;
                Sleep(250);
            }
        }

    }

    cout << endl << "Deploy Successfylly Completed !!!" << endl;
    eventLog("#############################Deploy Successfylly Completed !!!#############################");
    string program = ExePath();
    if(lastExecutionValue==0) {
        exec(program,"1");
    }
    else {
        system("pause");
    }
    return 0;
}


// function to get size of a file
int getLocalFileSize(string fileName)
{
    // open file in binary mode
    ifstream file(fileName, ios::binary | ios::ate);

    // get file size
    int fileSize = file.tellg();

    // close file
    file.close();

    return fileSize;
}


std::vector<std::string> listAllJsFiles(std::string path, std::string skipThisOne)
{
    std::vector<std::string> result;

    // Skip minified folder
    if(path==skipThisOne) {
        return result;
    }

    // WIN32_FIND_DATA structure is used for file attributes
    WIN32_FIND_DATA data;

    // creates a handle for the find operation
    HANDLE hFind = FindFirstFile((path + "\\*").c_str(), &data);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // skip the "." and ".." directories
            if (strcmp(data.cFileName, ".") == 0 ||
                strcmp(data.cFileName, "..") == 0)
                continue;

            // if it's a directory
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // call the function recursively
                std::vector<std::string> subResult =
                    listAllJsFiles(path + data.cFileName, skipThisOne);
                result.insert(result.end(),
                    subResult.begin(), subResult.end());
            }
            // if it's a file
            else
            {
                // check if the file is a JS file
                if (
                        std::string(data.cFileName).find(".js") != std::string::npos ||
                        std::string(data.cFileName).find(".css") != std::string::npos
                   )
                {
                    // add the file to the result
                    result.push_back(path + "\\" + data.cFileName);
                    // cout << path + "\\" + data.cFileName << endl;
                }
            }
        } while (FindNextFile(hFind, &data));

        FindClose(hFind);
    }

    return result;
}



void getFileName(string path, string* name, string *extension)
{
    string fileName;
    string fileExt;
    unsigned i = path.length()-1;
    while(i >= 0 && path[i] != '\\'){
        fileName.insert(0, 1, path[i]);
        i--;
    }
    i=0;
    std::string onlyName;
    while(i < fileName.size() && fileName[i] != '.'){
        onlyName+=fileName[i];
        i++;
    }
    i++; // to skip the point
    while(i < fileName.size()){
        fileExt+=fileName[i];
        i++;
    }
    *name = onlyName;
    *extension = fileExt;
}


std::string CopyFileWithMin(std::string sourcePath, std::string destinationPath){
    // Check for minified folder
    DWORD dwAttrib = GetFileAttributes(destinationPath.c_str());

    //If the path does not exist
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
        //Create the directory
        if (CreateDirectory(destinationPath.c_str(), NULL) == 0)
            return "";
    }

    // Get the file name from the source path
    std::string fileName = sourcePath.substr(sourcePath.find_last_of("\\") + 1);

    // Append .min to the file name
    fileName = "\\" + fileName.substr(0, fileName.find_last_of(".")) + ".min" + fileName.substr(fileName.find_last_of("."));

    // Append the file name to the destination path
    destinationPath.append(fileName);

    // Copy the file from the source to the destination
    if(CopyFile(sourcePath.c_str(), destinationPath.c_str(), false)){
        return destinationPath;
    } else {
        return "";
    }
}

void eventLog(std::string message) {
    std::ofstream logFile;
    logFile.open("log.log", std::ios_base::app);

    time_t now = time(0);
    tm *ltm = localtime(&now);

    string  year        = to_string(1900 + ltm->tm_year),
            month       = to_string(1 + ltm->tm_mon),
            day         = to_string(ltm->tm_mday),
            hour        = to_string(ltm->tm_hour),
            min         = to_string(ltm->tm_min),
            sec         = to_string(ltm->tm_sec);

    if(1 + ltm->tm_mon < 10) month = "0" + month;
    if(ltm->tm_mday < 10) day = "0" + day;
    if(ltm->tm_hour < 10) hour = "0" + hour;
    if(ltm->tm_min < 10) min = "0" + min;
    if(ltm->tm_sec < 10) sec = "0" + sec;

    logFile << "INFO "
            << year
            << "-"
            << month
            << "-"
            << day
            << "   "
            << hour
            << ":"
            << min
            << ":"
            << sec
            << "\t" + message
            << std::endl;
    logFile.close();
}


#include <iomanip>
void printProgressBar(double percent)
{
    // Set output stream precision
    cout << fixed << setprecision(2);

    //Get the console handle
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    //Get the current cursor position
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);

    // Variable to store the percentage
    float barSize = csbi.dwSize.X -9;

    // Print the percentage with a bar
    cout << "\r" << percent << "% [";
    for (int i = 0; i < barSize; i++)
    {
        if (i < (percent / 100) * barSize)
            cout << "=";
        else
            cout << " ";
    }
    cout << "]";
    cout << flush;
}

string ExePath() {
    char buffer[MAX_PATH] = { 0 };
    GetModuleFileName( NULL, buffer, MAX_PATH );
    string::size_type pos = string(buffer).find_last_of("\\/");
    return string(buffer).substr(0, pos) + "\\FtpDeployerQt.exe";
}


void exec(string program, string param) {
    SHELLEXECUTEINFOA ShExecInfo = {0};
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    // ShExecInfo.lpFile = std::wstring(program.begin(), program.end()).c_str();
    ShExecInfo.lpFile = program.c_str();
    ShExecInfo.lpParameters = param.c_str();
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_SHOW;
    ShExecInfo.hInstApp = NULL;
    ShellExecuteExA(&ShExecInfo);
    // WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
}

//void printLastLine(std::string text)
//{
//    //Get the console handle
//    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

//    //Get the current cursor position
//    CONSOLE_SCREEN_BUFFER_INFO csbi;
//    GetConsoleScreenBufferInfo(hConsole, &csbi);

//    //Move the cursor to the last line
//    COORD coord = {0, static_cast<SHORT>(csbi.dwSize.Y - 1)};
//    SetConsoleCursorPosition(hConsole, coord);

//    //Print the desired text
//    std::cout << text;
//}
