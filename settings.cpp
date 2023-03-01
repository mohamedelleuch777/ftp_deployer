#include "settings.h"
#include "definitions.h"

Settings::Settings()
{
    std::string filePath = getExePath() + "\\settings.ini";
    this->file = std::ifstream(filePath);
    if(!file.is_open()){
        //file.open(filePath,std::fstream::out);
        std::ofstream file(filePath);
        std::cout << "Could not open the INI file..." << std::endl << "Creating a new settings file..." << std::endl;
    } else {
        this->host              = this->read("HOST");
        this->username          = this->read("USERNAME");
        this->password          = this->read("PASSWORD");
        this->port              = this->read("PORT") == "" ? 0 : std::stoi(this->read("PORT"));
        this->localDirectory    = this->read("LOCAL_DIRECTORY");
        this->remoteDirectory   = this->read("REMOTE_DIRECTORY");
        this->minify            = this->read("MINIFY")=="TRUE";
        this->minifiedPath      = this->read("MINIFIED_PATH");
        this->maxAttempt        = this->read("MAX_ATTEMPT") == "" ? MAX_ATTEMPT : std::stoi(this->read("MAX_ATTEMPT"));
    }
}

Settings::~Settings()
{
    this->file.close();
}

std::string Settings::read(std::string key)
{
    std::string value;
    std::string line;
    this->file.seekg(0);
    while(getline(this->file, line))
    {
        if(line.size() > 0 && line[0] != '#')
        {
            std::size_t pos = line.find("=");
            // if(line.find(key) != std::string::npos)
            if(pos>=0)
            {
                std::string foundKey = line.substr(0,pos);
                if(foundKey==key) {
                    value = line.substr(pos+1);
                    break;
                }
            }
        }
    }
    return value;
}

std::string Settings::getExePath()
{
    // Get the current executable path
    char exeFile[MAX_PATH];
    GetModuleFileName(NULL, exeFile, MAX_PATH);

    // Print the executable path
    std::string path(exeFile);

    //Find the position of the last occurence of '/'
    const size_t last_slash_idx = path.find_last_of("/\\");
    //Return the substring up to the last slash
    return path.substr(0, last_slash_idx);

    return path;
}
