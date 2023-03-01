#ifndef SETTINGS_H
#define SETTINGS_H

#include <iostream>
#include <string>
#include <fstream>
#include<windows.h>

class Settings
{
    public:
        Settings();
        virtual ~Settings();
        std::string read(std::string key);



        std::string host;
        std::string username;
        std::string password;
        int         port;
        std::string localDirectory;
        std::string remoteDirectory;
        bool        minify;
        std::string minifiedPath;
        int         maxAttempt;

    protected:

    private:

        std::string getExePath();



        std::ifstream file;



};


#endif   // SETTINGS_H
