#ifndef WRAPPER_H
#define WRAPPER_H

#include <sstream>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <regex>
#include <vector>

// simple guard class for std::cerr stream capture
class cerrRedirect
{
public:
    cerrRedirect(std::streambuf *newBuffer)
    {
        oldBuffer = std::cerr.rdbuf(newBuffer);
    }
    ~cerrRedirect()
    {
        std::cerr.rdbuf(oldBuffer);
    }
private:
    std::streambuf *oldBuffer;
};

class Wrapper
{

public:
    explicit Wrapper();

    static std::string shellEscape(const std::string& arg);
    static std::string runCommand(const std::string& program, const std::vector<std::string>& args, int *exitStatus);
    static std::vector<std::filesystem::path> listFilesMatching(const std::filesystem::path& folder, const std::regex& pattern);
    static bool isExecutableFile(const std::filesystem::path& p);
    static std::string readFile(const std::string &path, std::string *errorMessage);
    static std::string toString(const char * const printfFormatString, ...);


private:

    std::string m_parameterFile;
    std::string m_startingPopulationFile;
    std::string m_xmlMasterFile;
    std::string m_outputFolder;

    std::string m_gaExecutable;
    std::string m_gaitSymExecutable;

    std::string m_modelConfigurationFile;
    std::string m_modelPopulationFile;
    std::string m_driverFile;
    std::string m_mergeXMLFile;
    std::string m_workingFolder;
    std::string m_postMergeScript;

    double m_startValue =0;
    double m_stepValue = 1;
    double m_endValue = 10;
    double m_outputCycle = 0;
    bool m_cycleFlag = false;

    int m_logLevel = 1;
    int m_serverPort = 8086;

    double m_currentLoopValue = 0;
    int m_currentLoopCount = 0;
    std::chrono::time_point<std::chrono::steady_clock> m_lastResultsTime;
    int m_lastResultsNumber = -1;

    std::string m_startExpressionMarker = {"[["};
    std::string m_endExpressionMarker = {"]]"};

    void runPostMergeScript();

    void runGA();
    void runGaitSym();
    void openFile(const std::string &fileName);
    void runMergeXML();
    void readStandardError();
    void readStandardOutput();


    std::stringstream m_capturedCerr;
    std::unique_ptr<cerrRedirect> m_redirect;

};

#endif // WRAPPER_H
