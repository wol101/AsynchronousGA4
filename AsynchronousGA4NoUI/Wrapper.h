#ifndef WRAPPER_H
#define WRAPPER_H

#include <sstream>
#include <chrono>
#include <filesystem>
#include <regex>
#include <vector>

class Wrapper
{

public:
    explicit Wrapper(const std::string &settingsFile);

    void run();

    static std::string shellEscape(const std::string& arg);
    static std::string runCommand(const std::string& program, const std::vector<std::string>& args, int *exitStatus);
    static std::vector<std::filesystem::path> listFilesMatching(const std::filesystem::path& folder, const std::regex& pattern);
    static bool isExecutableFile(const std::filesystem::path& p);
    static std::string readFile(const std::string &path, std::string *errorMessage);
    static std::string toString(const char * const printfFormatString, ...);
    static std::filesystem::path existsOnPath(const std::string& filename);
    static bool toBool(const std::string& s, bool *valid = 0);


    void setLogLevel(int newLogLevel);

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
    bool m_mergeXMLActivate = false;
    bool m_cycle = false;

    int m_logLevel = 1;
    int m_portNumber = 8086;

    double m_currentLoopValue = 0;
    int m_currentLoopCount = 0;
    std::chrono::time_point<std::chrono::steady_clock> m_lastResultsTime;
    int m_lastResultsNumber = -1;

    std::string m_startExpressionMarker = {"[["};
    std::string m_endExpressionMarker = {"]]"};

    void runPostMergeScript();
    void runGA();
    void runGaitSym();
    void openSettingsFile(const std::string &fileName);
    void runMergeXML();


};

#endif // WRAPPER_H
