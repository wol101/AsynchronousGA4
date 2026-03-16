#include "Wrapper.h"

#include "MergeUtil.h"
#include "MergeXML.h"
#include "XMLConverter.h"

#include "pystring.h"

#include <filesystem>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <algorithm>

#if defined(_WIN32)
#else
#include <unistd.h>
#endif



using namespace std::string_literals;

Wrapper::Wrapper()
{
}

void Wrapper::run()
{
    if (m_mergeXMLActivate) runMergeXML();
    else runGA();
}

void Wrapper::runGA()
{
    if (m_logLevel > 0) std::cerr << "Running runGA\n";
    if (!isExecutableFile(m_gaExecutable))
    {
        std::cerr << "Error: runGaitSym: Unable to run \"" << m_gaExecutable << "\"\n";
        std::exit(1);
    }
    std::vector<std::string> arguments{"--parameterFile", m_parameterFile,
                                       "--baseXMLFile", m_xmlMasterFile,
                                       "--startingPopulation", m_startingPopulationFile,
                                       "--outputDirectory", m_outputFolder,
                                       "--serverPort", std::to_string(m_portNumber),
                                       "--logLevel", std::to_string(m_logLevel)};
    if (m_logLevel > 0) std::cerr << "Running \"" << m_gaExecutable << "\"\n";
    if (m_logLevel > 1) std::cerr << pystring::join(" "s, arguments) << "\n";
    int status = runCommand(m_gaExecutable, arguments);
    if (status)
    {
        std::cerr << "Error " << status << " running: " << m_gaExecutable << pystring::join(" "s, arguments) << "\n";
        std::exit(status);
    }
}

void Wrapper::runMergeXML()
{
    if (m_logLevel > 0) std::cerr << "Running runMergeXML\n";
    // get the population and config from the models
    std::filesystem::path lastPopulation = m_modelPopulationFile;
    std::filesystem::path lastConfig = m_modelConfigurationFile;
    bool firstLoop = true;
    m_maxLoopCount = static_cast<int>(std::round((m_endValue - m_startValue) / m_stepValue));
    m_currentLoopValue = m_startValue;
    m_currentLoopCount = 0;

    while (m_currentLoopCount <= m_maxLoopCount)
    {
        if (m_logLevel > 1)
        {
            std::cerr << "m_currentLoopCount = " << m_currentLoopCount << " m_currentLoopCount = " << m_currentLoopCount << "\n";
            std::cerr << "lastPopulation = " << lastPopulation << " lastConfig = " << lastConfig << "\n";
        }
        std::filesystem::path driverFile(m_driverFile);
        std::filesystem::path workingFolder(m_workingFolder);
        std::string errorMessage;
        if (m_logLevel > 1) std::cerr << "Reading \"" << m_mergeXMLFile << "\"\n";
        std::string mergeXMLCommands = readFileToString(m_mergeXMLFile, &errorMessage);
        if (!errorMessage.empty())
        {
            std::cerr << "Error: MergeXML: Unable to open file:\"" << m_mergeXMLFile << "\"\n";
            std::exit(1);
        }
#if ( __GNUC__ >= 14 ) || ( _MSC_VER >= 1929 ) // these versions required for std::format and std::chrono::current_zone support for C++20
        auto currentTime = std::chrono::system_clock::now();
        auto localSecondsTime = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::current_zone()->to_local(currentTime)); //needs to be cast to seconds otherwise %S has decimal digits
        std::string timeString = std::format("{:%Y-%m-%d_%H.%M.%S}", localSecondsTime);
#else
        time_t now = time(nullptr);
        struct tm local;
#ifdef _MSC_VER
        localtime_s(&now, &local);
#else
        localtime_r(&now, &local);
#endif
        std::string timeString = toString("%04d-%02d-%02d_%02d.%02d.%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
#endif

        std::filesystem::path subFolder(toString("%04d_Run_%s", m_currentLoopCount, timeString.c_str()));
        std::filesystem::path outputFolder = workingFolder / subFolder;
        m_outputFolder = toAbsolutePath(outputFolder);
        if (m_logLevel > 1) std::cerr << "Creating " << outputFolder << "\n";
        std::filesystem::create_directories(outputFolder);
        if (!std::filesystem::is_directory(outputFolder))
        {
            std::cerr << "Error: MergeXML: Unable to create folder: \"" << outputFolder << "\"\n";
            std::exit(1);
        }
        std::filesystem::path workingConfig = (outputFolder / "workingConfig.xml");
        m_xmlMasterFile = toAbsolutePath(workingConfig);
        std::filesystem::path firstConfigFile = m_modelConfigurationFile;
        std::string replace = pystring::replace(mergeXMLCommands, "MODEL_CONFIG_FILE", toAbsolutePath(lastConfig));
        replace = pystring::replace(replace, "DRIVER_CONFIG_FILE", toAbsolutePath(driverFile));
        replace = pystring::replace(replace, "WORKING_CONFIG_FILE", toAbsolutePath(workingConfig));
        replace = pystring::replace(replace, "CURRENT_LOOP_VALUE", toString("%.*gs", 17, m_currentLoopValue));
        replace = pystring::replace(replace, "FIRST_CONFIG_FILE", toAbsolutePath(firstConfigFile));
        MergeXML mergeXML;
        mergeXML.ExecuteInstructionFile(replace);
        if (mergeXML.errorMessageList().size())
        {
            std::cerr << "Error: MergeXML: Unable to parse file: \"" << m_mergeXMLFile << "\"\n";
            std::exit(1);
        }
        std::filesystem::path newMergeXML = outputFolder / "workingMergeXML.txt";
        if (m_logLevel > 1) std::cerr << "Writing " << newMergeXML << "\n";
        std::ofstream file(newMergeXML, std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Error: MergeXML: Open File Error: \"" << newMergeXML << "\" " << std::error_code(errno, std::system_category()).message() << "\n";
            std::exit(1);
        }
        file.write(replace.c_str(), replace.size());
        file.close();
        std::filesystem::path mergeXMLStatusFile = outputFolder / "mergeXMLStatus.txt";
        if (m_logLevel > 1) std::cerr << "Writing " << mergeXMLStatusFile << "\n";
        std::ofstream stream(mergeXMLStatusFile, std::ios::out | std::ios::binary);
        if (!stream.is_open())
        {
            std::cerr << "Error: MergeXML: Open File Error: \"" << mergeXMLStatusFile << "\" " << std::error_code(errno, std::system_category()).message() << "\n";
            std::exit(1);
        }
        stream << "startValue " << m_startValue << "\n";
        stream << "stepValue " << m_stepValue << "\n";
        stream << "endValue " << m_endValue << "\n";
        stream << "currentLoopCount " << m_currentLoopCount << "\n";
        if (m_cycle) stream << "outputCycle " << m_outputCycle << "\n";
        else stream << "outputTime " << m_outputCycle << "\n";
        stream << "CURRENT_LOOP_VALUE " << m_currentLoopValue << "\n";
        stream << "MODEL_CONFIG_FILE " << lastConfig << "\n";
        stream << "DRIVER_CONFIG_FILE " << driverFile << "\n";
        stream << "WORKING_CONFIG_FILE " << workingConfig << "\n";
        stream << "FIRST_CONFIG_FILE " << firstConfigFile << "\n";
        stream.close();
        std::filesystem::path newPopulation = outputFolder / "workingPopulation.txt";
        if (m_logLevel > 1) std::cerr << "Copying " << lastPopulation << " to " << newPopulation << "\n";
        std::filesystem::copy_file(lastPopulation, newPopulation);
        m_startingPopulationFile = toAbsolutePath(newPopulation);
        runPostMergeScript();
        runGA();
        runGaitSym();
        lastConfig = (outputFolder / "ModelState.xml");
        if (!std::filesystem::exists(lastConfig))
        {
            std::cerr << "Error: MergeXML: Unable to find ModelState.xml in: \"" << m_outputFolder << "\"\n";
            exit(1);
        }
        std::vector<std::filesystem::path> files = listFilesMatching(outputFolder, std::regex("^Population.*\\.txt$"));
        if (files.size() == 0)
        {
            std::cerr << "Error: MergeXML: Unable to find ^Population.*\\.txt$ in: \"" << m_outputFolder << "\"\n";
            exit(1);
        }
        lastPopulation = files.back();
        m_currentLoopCount++;
        m_currentLoopValue = m_startValue + m_currentLoopCount * m_stepValue;
    }
}

void Wrapper::runPostMergeScript()
{
    if (m_postMergeScript.empty()) return;

    if (isExecutableFile(m_postMergeScript))
    {
        std::vector<std::string> arguments{"--startingPopulationFile", m_startingPopulationFile,
                "--xmlMasterFile", m_xmlMasterFile,
                "--outputFolder", m_outputFolder,
                "--currentLoopValue", toString("%.*g", 17, m_currentLoopValue),
                "--logLevel", std::to_string(m_logLevel)};

        if (m_logLevel > 0) std::cerr << "Running \"" << m_postMergeScript << "\"\n";
        if (m_logLevel > 1) std::cerr << pystring::join(" "s, arguments) << "\n";
        int returnCode = runCommand(m_postMergeScript, arguments);
        if (returnCode)
        {
            std::cerr << "Error " << returnCode << " running \"" << m_postMergeScript << "\"\n";
            std::exit(1);
        }
    }
    else
    {
        std::filesystem::path interpreter;
        if (pystring::endswith(pystring::lower(m_postMergeScript), ".py"))
        {
            while (interpreter.empty())
            {
                interpreter = existsOnPath("python");
                if (!interpreter.empty()) break;
                interpreter = existsOnPath("python3");
                if (!interpreter.empty()) break;
                std::vector<std::string> options{"C:/ProgramData/miniconda3/python.exe", "C:/ProgramData/anaconda3/python.exe", "C:/Program Files/Spyder/Python/python.exe"};
                for (auto &&option : options)
                {
                    if (isExecutableFile(option))
                    {
                        interpreter = option;
                        break;
                    }
                }
                break;
            }
        }
        if (pystring::endswith(pystring::lower(m_postMergeScript), ".r"))
        {
            interpreter = existsOnPath("Rscript");
        }
        if (interpreter.empty())
        {
            std::cerr << "Error: runPostMergeScript: Unknown script \"" << m_postMergeScript << "\"\n";;
            std::exit(1);
        }
        std::vector<std::string> arguments{m_postMergeScript,
                                           "--startingPopulationFile", m_startingPopulationFile,
                                           "--xmlMasterFile", m_xmlMasterFile,
                                           "--outputFolder", m_outputFolder,
                                           "--currentLoopValue", toString("%.*g", 17, m_currentLoopValue),
                                           "--logLevel", std::to_string(m_logLevel)};        
        if (m_logLevel > 0) std::cerr << "Running \"" << m_postMergeScript << "\" \"" << arguments.front() << "\"\n";
        if (m_logLevel > 1) std::cerr << pystring::join(" "s, std::vector<std::string>(arguments.begin() + 1, arguments.end())) << "\n";
        int returnCode = runCommand(toAbsolutePath(interpreter), arguments);
        if (returnCode)
        {
            std::cerr << "Error " << returnCode << " running \"" << m_postMergeScript << "\" \"" << arguments.front() << "\"\n";
            std::exit(1);
        }
    }
}

void Wrapper::runGaitSym()
{
    if (m_logLevel > 0) std::cerr << "Running GaitSym\n";
    std::filesystem::path dir(m_outputFolder);
    std::vector<std::filesystem::path> files = listFilesMatching(dir, std::regex("^BestGenome.*\\.txt$"));
    if (files.size() < 1)
    {
        std::cerr << "Error: runGaitSym: Unable to find ^BestGenome.*\\.txt$\n";
        std::exit(1);
    }
    std::filesystem::path inputGenome = files.back();
    std::filesystem::path inputXML = dir / "workingConfig.xml";
    std::filesystem::path outputXML = inputGenome;
    outputXML.replace_extension(".xml");
    std::vector<double> genes, lowBounds, highBounds, gaussianSDs;
    std::vector<int> circularMutationFlags;
    double fitness;
    int genomeType;
    if (m_logLevel > 1) std::cerr << "Reading genome file " << inputGenome << "\n";
    MergeUtil::readGenome(toAbsolutePath(inputGenome), &genes, &lowBounds, &highBounds, &gaussianSDs, &circularMutationFlags, &fitness, &genomeType);
    XMLConverter xmlConverter;
    if (m_logLevel > 1) std::cerr << "Reading base XML file " << inputXML << "\n";
    xmlConverter.LoadBaseXMLFile(toAbsolutePath(inputXML));
    xmlConverter.ApplyGenome(genes);
    std::string formattedXML;
    xmlConverter.GetFormattedXML(&formattedXML);
    if (m_logLevel > 1) std::cerr << "Writing new XML file " << outputXML << "\n";
    std::ofstream outputXMLFile(outputXML, std::ios::binary);
    outputXMLFile.write(formattedXML.data(), formattedXML.size());
    outputXMLFile.close();

    std::filesystem::path modelStateFileName = dir / "ModelState.xml";
    if (!isExecutableFile(m_gaitSymExecutable))
    {
        std::cerr << "Error: runGaitSym: Unable to run \"" << m_gaitSymExecutable << "\"\n";
        std::exit(1);
    }
    std::vector<std::string> arguments;
    arguments.push_back("--config");
    arguments.push_back(toAbsolutePath(outputXML));
    if (m_cycle) arguments.push_back("--outputModelStateAtCycle");
    else arguments.push_back("--outputModelStateAtTime");
    arguments.push_back(toString("%.*g", 17, m_outputCycle)); // this lets you set the precision as an argument
    arguments.push_back("--modelState");
    arguments.push_back(toAbsolutePath(modelStateFileName));
    if (m_logLevel > 0) std::cerr << "Running \"" << m_gaitSymExecutable << "\"\n";
    if (m_logLevel > 1) std::cerr << pystring::join(" "s, arguments) << "\n";
    int exitStatus = runCommand(m_gaitSymExecutable, arguments);
    if (exitStatus)
    {
        std::cerr << "Error: runGaitSym: fail running \"" << m_gaitSymExecutable << "\"\n";
        return;
    }
    if (!std::filesystem::exists(modelStateFileName) && !std::filesystem::is_regular_file(modelStateFileName))
    {
        std::cerr << "Error :runGaitSym: Unable to create: " << modelStateFileName << "\n";
        exit(1);
    }
}

void Wrapper::openSettingsFile(const std::string &fileName)
{
    if (m_logLevel > 0) std::cerr << "Opening \"" << fileName << "\"\n";
    XMLContainer xmlContainer;
    if (xmlContainer.LoadXML(fileName))
    {
        std::cerr << "Error: openFile LoadXML \"" << fileName << "\\n";
        std::exit(1);
    }

    char *attributePtr, *endPtr;
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "cycle"); if (attributePtr) m_cycle = toBool(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "driverFile"); if (attributePtr) m_driverFile = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "endExpressionMarker"); if (attributePtr) m_endExpressionMarker = attributePtr;
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "endValue"); if (attributePtr) m_endValue = std::strtof(attributePtr, &endPtr);;
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "gaExecutable"); if (attributePtr) m_gaExecutable = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "gaitSymExecutable"); if (attributePtr) m_gaitSymExecutable = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "logLevel"); if (attributePtr && !m_overrideLogLevel) m_logLevel = std::strtol(attributePtr, &endPtr, 10);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "mergeXMLActivate"); if (attributePtr) m_mergeXMLActivate = toBool(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "mergeXMLFile"); if (attributePtr) m_mergeXMLFile = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "modelConfigurationFile"); if (attributePtr) m_modelConfigurationFile = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "modelPopulationFile"); if (attributePtr) m_modelPopulationFile = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "outputCycle"); if (attributePtr) m_outputCycle = std::strtof(attributePtr, &endPtr);;
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "outputFolder"); if (attributePtr) m_outputFolder = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "parameterFile"); if (attributePtr) m_parameterFile = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "portNumber"); if (attributePtr) m_portNumber = std::strtol(attributePtr, &endPtr, 10);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "postMergeScript"); if (attributePtr && std::strlen(attributePtr)) m_postMergeScript = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "startExpressionMarker"); if (attributePtr) m_startExpressionMarker = attributePtr;
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "startValue"); if (attributePtr) m_startValue = std::strtof(attributePtr, &endPtr);;
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "startingPopulationFile"); if (attributePtr) m_startingPopulationFile = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "stepValue"); if (attributePtr) m_stepValue = std::strtof(attributePtr, &endPtr);;
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "workingFolder"); if (attributePtr) m_workingFolder = toAbsolutePath(attributePtr);
    attributePtr =  xmlContainer.DoXmlGetProp("SETTINGS", 0, 0, "xmlMasterFile"); if (attributePtr) m_xmlMasterFile = toAbsolutePath(attributePtr);

    m_maxLoopCount = static_cast<int>(std::round((m_endValue - m_startValue) / m_stepValue));
    m_currentLoopValue = m_startValue;

    if (m_logLevel > 1)
    {
        std::cerr
                  << "m_currentLoopCount = "       << m_currentLoopCount << '\n'
                  << "m_currentLoopValue = "       << m_currentLoopValue << '\n'
                  << "m_cycle = "                  << std::boolalpha << m_cycle << '\n'
                  << "m_driverFile = "             << m_driverFile << '\n'
                  << "m_endExpressionMarker = "    << m_endExpressionMarker << '\n'
                  << "m_endValue = "               << m_endValue << '\n'
                  << "m_gaExecutable = "           << m_gaExecutable << '\n'
                  << "m_gaitSymExecutable = "      << m_gaitSymExecutable << '\n'
                  << "m_logLevel = "               << m_logLevel << '\n'
                  << "m_mergeXMLActivate = "       << std::boolalpha << m_mergeXMLActivate << '\n'
                  << "m_mergeXMLFile = "           << m_mergeXMLFile << '\n'
                  << "m_modelConfigurationFile = " << m_modelConfigurationFile << '\n'
                  << "m_modelPopulationFile = "    << m_modelPopulationFile << '\n'
                  << "m_outputCycle = "            << m_outputCycle << '\n'
                  << "m_outputFolder = "           << m_outputFolder << '\n'
                  << "m_overrideLogLevel = "       << std::boolalpha << m_overrideLogLevel << '\n'
                  << "m_parameterFile = "          << m_parameterFile << '\n'
                  << "m_portNumber = "             << m_portNumber << '\n'
                  << "m_postMergeScript = "        << m_postMergeScript << '\n'
                  << "m_startExpressionMarker = "  << m_startExpressionMarker << '\n'
                  << "m_startValue = "             << m_startValue << '\n'
                  << "m_startingPopulationFile = " << m_startingPopulationFile << '\n'
                  << "m_stepValue = "              << m_stepValue << '\n'
                  << "m_workingFolder = "          << m_workingFolder << '\n'
                  << "m_xmlMasterFile = "          << m_xmlMasterFile << '\n';
    }
}



// Returns the full path if found, or an empty path if not found.
std::filesystem::path Wrapper::existsOnPath(const std::filesystem::path& filename)
{
#ifdef _WIN32
    static const char PATH_SEPARATOR = ';';
#else
    static const char PATH_SEPARATOR = ':';
#endif

    const char* path_env = std::getenv("PATH");
    if (!path_env)
    {
        return {};
    }

    std::string path_str(path_env);
    std::vector<std::string> dirs;

    // Split PATH
    size_t start = 0;
    while (true)
    {
        size_t end = path_str.find(PATH_SEPARATOR, start);
        if (end == std::string::npos)
        {
            dirs.push_back(path_str.substr(start));
            break;
        }
        dirs.push_back(path_str.substr(start, end - start));
        start = end + 1;
    }

    // Search each directory
    for (const auto& dir : dirs)
    {
        std::filesystem::path candidate = std::filesystem::path(dir) / filename;

#ifdef _WIN32
        // On Windows, executables may need .exe if not provided
        if (!candidate.has_extension())
        {
            std::filesystem::path exe_candidate = candidate;
            exe_candidate += ".exe";
            if (std::filesystem::exists(exe_candidate) && std::filesystem::is_regular_file(exe_candidate))
            {
                return exe_candidate;
            }
        }
#endif

        if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate))
        {
            return candidate;
        }
    }

    return {};
}




std::string Wrapper::shellEscape(const std::string& arg)
{
    std::string out;
    out.reserve(arg.size() + 2);

    out.push_back('\'');
    for (char c : arg)
    {
        if (c == '\'')
        {
            // End quote, insert escaped single quote, reopen quote
            out += "'\"'\"'";
        }
        else
        {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

int Wrapper::runCommand(const std::string& program, const std::vector<std::string>& args, std::string *result)
{
    std::ostringstream cmd;
    cmd << shellEscape(program);

    for (auto& a : args)
    {
        cmd << " " << shellEscape(a);
    }

    if (result)
    {
        cmd << " 2>&1";  // redirect stderr to stdout
        result->clear();

#if defined(_WIN32)
        FILE* pipe = _popen(cmd.str().c_str(), "r");
#else
        FILE* pipe = popen(cmd.str().c_str(), "r");
#endif

        if (!pipe) { return -1; }

        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), pipe)) { *result += buffer; }

#if defined(_WIN32)
        return _pclose(pipe);
#else
        return pclose(pipe);
#endif
    }
    return std::system(cmd.str().c_str());
}


std::vector<std::filesystem::path> Wrapper::listFilesMatching(const std::filesystem::path& folderPath, const std::regex& pattern)
{
    std::vector<std::filesystem::path> results;

    if (!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath)) {
        return results; // return empty if folder doesn't exist
    }

    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::string filename = entry.path().filename().string();

        if (std::regex_match(filename, pattern)) {
            results.push_back(entry.path());
        }
    }

    return results;
}

bool Wrapper::isExecutableFile(const std::filesystem::path& filePath)
{
    // Must exist and be a regular file
    if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath)) {
        return false;
    }

#if defined(_WIN32)
    // Windows: no executable bit, so we check common executable extensions
    std::string ext = filePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".exe" ||
           ext == ".bat" ||
           ext == ".cmd" ||
           ext == ".com" ||
           ext == ".ps1";
#else
    // POSIX: check execute permission for the current user
    return ::access(filePath.c_str(), X_OK) == 0;
#endif
}

std::string Wrapper::readFileToString(const std::string &pathString, std::string *errorMessage)
{
    std::string result;
    std::ifstream file(pathString, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        if (errorMessage)
        {
            switch (errno)
            {
            case ENOENT: *errorMessage = "File Not Found"; break;
            case EACCES: *errorMessage = "Permission Denied"; break;
            case EISDIR: *errorMessage = "Is Directory"; break;
            default:     *errorMessage = "Unknown Error"; break;
            }
        }
        return result;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    result = ss.str();

    if (file.bad())
    {
        if (errorMessage) { *errorMessage = "I/O error while reading file: \""s + pathString + "\n"s; }
        return result;
    }

    return result;
}

// convert to string using printf style formatting and variable numbers of arguments
std::string Wrapper::toString(const char * const printfFormatString, ...)
{
    // initialize use of the variable argument array
    va_list vaArgs;
    va_start(vaArgs, printfFormatString);

    // reliably acquire the size
    // from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy(vaArgsCopy, vaArgs);
    const int iLen = std::vsnprintf(nullptr, 0, printfFormatString, vaArgsCopy);
    va_end(vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    std::unique_ptr<char[]> zc = std::make_unique<char[]>(iLen + 1);
    std::vsnprintf(zc.get(), size_t(iLen + 1), printfFormatString, vaArgs);
    va_end(vaArgs);
    return std::string(zc.get(), size_t(iLen));
}

bool Wrapper::toBool(const std::string& s, bool *valid)
{
    std::string v = s;
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);

    if (v == "true"  || v == "1" || v == "yes" || v == "on")
    {
        if (valid) *valid = true;
        return true;
    }

    if (v == "false" || v == "0" || v == "no"  || v == "off")
    {
        if (valid) *valid = true;
        return false;
    }

     if (valid) *valid = true;
    return false;
}

std::string Wrapper::toAbsolutePath(const std::string &pathString)
{
    std::string absolutePathString;
    std::filesystem::path varname(pathString);
    absolutePathString = std::filesystem::absolute(pathString).string();
    return absolutePathString;
}

void Wrapper::setLogLevel(int newLogLevel)
{
    m_logLevel = newLogLevel;
    m_overrideLogLevel = true;

}


