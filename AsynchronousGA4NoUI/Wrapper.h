#ifndef WRAPPER_H
#define WRAPPER_H

#include <filesystem>
#include <regex>
#include <vector>

class Wrapper
{

public:
    explicit Wrapper();

    void openSettingsFile(const std::string &fileName);
    void run();

    static std::string shellEscape(const std::string& arg);
    static int runCommand(const std::string& program, const std::vector<std::string>& args, std::string *result = 0);
    static std::vector<std::filesystem::path> listFilesMatching(const std::filesystem::path& folderPath, const std::regex& pattern);
    static bool isExecutableFile(const std::filesystem::path& filePath);
    static std::string readFileToString(const std::string &pathString, std::string *errorMessage);
    static std::string toString(const char * const printfFormatString, ...);
    static std::filesystem::path existsOnPath(const std::filesystem::path &filename);
    static bool toBool(const std::string& s, bool *valid = 0);
    static std::string toCanonicalPath(const std::filesystem::path &path);
    static std::string toString(const std::u8string &u8);

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
    double m_stepValue = 0;
    double m_endValue = 0;
    double m_outputCycle = 0;
    bool m_mergeXMLActivate = false;
    bool m_cycle = false;

    int m_logLevel = 0;
    bool m_overrideLogLevel = false;
    int m_portNumber = 0;

    double m_currentLoopValue = 0;
    int m_currentLoopCount = 0;
    int m_maxLoopCount = 0;

    std::string m_startExpressionMarker = {"[["};
    std::string m_endExpressionMarker = {"]]"};

    void runPostMergeScript();
    void runGA();
    void runGaitSym();
    void runMergeXML();


};

#endif // WRAPPER_H
