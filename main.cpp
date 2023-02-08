
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>
#include "cftp.h"
#include "settings.h"
#include <dirent.h>
#include "definitions.h"

using namespace std;

int requestFileSize(string fileName);
int getLocalFileSize(string fileName);
std::vector<std::string> listAllJsFiles(std::string path);
void getFileName(string path, string* name, string *extension);
bool CopyFileWithMin(std::string sourcePath, std::string destinationPath);

cFTP ftp;


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
    for (int i = 0; i < fileList.size(); i++)
    {
        // Get file names
        fileName = fileList[i];
        remoteFileName = remoteFolder + fileName;
        localFileName = localFolder + fileName;

        // Prepare file to be copied
        if(st.minify) {
            // Minify the file
        } else {
            // Copy it to the minified path with adding .min to the name
            string minPath = st.minifiedPath;
            CopyFileWithMin(fileName, minPath);
        }

        // Retry uploading file until file sizes match
        bool uploadComplete = false;

            cout << "Trying to send: " << fileName << endl;
        while (!uploadComplete)
        {
            // Upload file
            bool uploadResult = ftp.sendFile(localFileName.c_str(), remoteFileName.c_str(), cFTP::MODE_ASCII);
            if (uploadResult == false) {
                std::string err = "Send File failed: " + GetLastError();
                throw std::runtime_error(err);
                return false;
            }

            // Get file sizes
            localFileSize = getLocalFileSize(localFileName);
            remoteFileSize = ftp.getFileSize(remoteFileName);
            // remoteFileSize = requestFileSize(remoteFileName);

            // Check if file sizes match
            if (localFileSize == remoteFileSize)
                uploadComplete = true;
        }

        // Print success message
        cout << "Successfully uploaded " << fileName << endl;
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


std::vector<std::string> listAllJsFiles(std::string path)
{
    std::vector<std::string> result;

    // WIN32_FIND_DATA structure is used for file attributes
    WIN32_FIND_DATAA data;

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
                if (std::string(data.cFileName).find(".js") != std::string::npos)
                {
                    // add the file to the result
                    result.push_back(path + "\\" + data.cFileName);
                    cout << path + "\\" + data.cFileName << endl;
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


bool CopyFileWithMin(std::string sourcePath, std::string destinationPath){
    // Check for minified folder
    DWORD dwAttrib = GetFileAttributes(destinationPath.c_str());

    //If the path does not exist
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
        //Create the directory
        if (CreateDirectory(destinationPath.c_str(), NULL) == 0)
            return false;
    }

    // Get the file name from the source path
    std::string fileName = sourcePath.substr(sourcePath.find_last_of("\\") + 1);

    // Append .min to the file name
    fileName = fileName.substr(0, fileName.find_last_of(".")) + ".min" + fileName.substr(fileName.find_last_of("."));

    // Append the file name to the destination path
    destinationPath.append(fileName);

    // Copy the file from the source to the destination
    if(CopyFile(sourcePath.c_str(), destinationPath.c_str(), false)){
        return true;
    } else {
        return false;
    }
}
