#ifndef WRAPPER_H
#define WRAPPER_H

#include <sstream>
#include <iostream>
#include <chrono>

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

    void run();

    static std::string shellEscape(const std::string& arg);
    static int runCommand(const std::string& program, const std::vector<std::string>& args);


private:
    std::string convertToRelativePath(const std::string &filename);
    std::string convertToAbsolutePath(const std::string &filename);
    std::string existsOnPath(const std::string &filename);

    double m_startValue =0;
    double m_stepValue = 1;
    double m_endValue = 10;
    std::string m_modelPopulationFile;
    std::string m_modelConfigurationFile;

    double m_currentLoopValue = 0;
    int m_currentLoopCount = 0;
    std::chrono::time_point<std::chrono::steady_clock> m_lastResultsTime;
    int m_lastResultsNumber = -1;
    std::string m_lastPopulation;
    std::string m_lastConfig;


    std::string m_asynchronousGAFileName;

    std::string m_outputFolder;
    std::string m_parameterFile;
    std::string m_xmlMasterFile;
    std::string m_startingPopulationFile;
    int m_logLevel = 1;
    int serverPort = 8086;

    std::string gaExecutable;

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
