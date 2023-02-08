
#define MAX_ATTEMPT         3

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>
#include "cftp.h"
#include "settings.h"
#include <dirent.h>
#include "definitions.h"
#include <ctime>

using namespace std;

int requestFileSize(string fileName);
int getLocalFileSize(string fileName);
std::vector<std::string> listAllJsFiles(std::string path);
void getFileName(string path, string* name, string *extension);
std::string CopyFileWithMin(std::string sourcePath, std::string destinationPath);
void eventLog(std::string message);

cFTP ftp;
int attempt = 0;

int main()
{
    Settings st;
    ftp.setHost(st.host);
    ftp.setUsername(st.username);
    ftp.setPassword(st.password);
    ftp.setPort(st.port);
    if(!ftp.Connect()) {
        cout << "FTP CONNECT FAILED" << endl;
        return 5;
    }
    cout << "CONNECTED TO SERVER...\nSTARTING DEPLOY" << endl;
    // Declare variables
    vector<string> fileList;
    std::string remoteFolder = st.remoteDirectory;
    std::string localFolder = st.localDirectory;
    string fileName, remoteFileName, localFileName;
    long localFileSize, remoteFileSize;

    // Get the file list of the directory
    fileList = listAllJsFiles(localFolder);

    // Upload files
    for (unsigned i = 0; i < fileList.size(); i++)
    {
        // Get file names
        fileName = fileList[i];


        //Find the position of the substring
        int pos = fileName.find(localFolder);
        //Calculate the length of the substring
        int len = localFolder.size() - pos + 1;
        //Remove the substring
        fileName.erase(pos, len);


        localFileName = localFolder + fileName;
        std::string nm, ex;
        getFileName(fileName, &nm, &ex);
        remoteFileName = remoteFolder + nm + ".min." + ex;

        string minifiedFilePath = "";

        // Prepare file to be copied
        if(st.minify) {
            // Minify the file
        } else {
            // Copy it to the minified path with adding .min to the name
            string minPath = st.minifiedPath;
            minifiedFilePath = CopyFileWithMin(localFileName, minPath);
        }

        // Retry uploading file until file sizes match
        bool uploadComplete = false;

            cout << "Trying to send: " << fileName << endl;
        while (!uploadComplete)
        {
            // Upload file
            bool uploadResult = ftp.sendFile(minifiedFilePath.c_str(), remoteFileName.c_str(), cFTP::MODE_ASCII);
            if (!uploadResult) {
                unsigned errCode = GetLastError();
                std::string err = "Send File failed: " + std::to_string(errCode);
                if(attempt>MAX_ATTEMPT) {
                    eventLog("Deploy was not completed !!!");
                    throw std::runtime_error(err);
                    return errCode;
                }
                else {
                    cout << "error code: " << errCode << endl;
                    cout << err << endl;
                    cout << "Retry to send the same file again. Attempt: " << attempt << endl;
                    eventLog("Failed Error: " + std::to_string(errCode) + ". Retry " + std::to_string(attempt) + ": " + fileName);
                    attempt++;
                    if(errCode==12030 || errCode==12111) {
                        ftp.Reconnect();
                    }
                    continue;
                }
            }

            // Reset the attempts counter
            attempt = 0;

            // Get file sizes
            localFileSize = getLocalFileSize(localFileName);
            remoteFileSize = ftp.getFileSize(remoteFileName);
            // remoteFileSize = requestFileSize(remoteFileName);

            // Check if file sizes match
            if (localFileSize == remoteFileSize)
                uploadComplete = true;
        }

        // Print success message
        cout << "Successfully uploaded: " << fileName << endl;
        eventLog("Successfully uploaded: " + fileName);
    }

    cout << "Deploy Successfylly Completed !!!" << endl;
    eventLog("Deploy Successfylly Completed !!!");
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


std::vector<std::string> listAllJsFiles(std::string path)
{
    std::vector<std::string> result;

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
                    listAllJsFiles(path + "\\" + data.cFileName);
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
    int i = path.length()-1;
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
    logFile.open("log.txt", std::ios_base::app);

    time_t now = time(0);
    tm *ltm = localtime(&now);

    logFile << "INFO "
            << 1900 + ltm->tm_year
            << "-"
            << 1 + ltm->tm_mon
            << "-"
            << ltm->tm_mday
            << "   "
            << ltm->tm_hour
            << ":"
            << ltm->tm_min
            << ":"
            << ltm->tm_sec
            << message
            << std::endl;
    logFile.close();
}
