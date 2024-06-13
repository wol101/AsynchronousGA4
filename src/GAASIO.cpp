/*
 *  GAMain.cpp
 *  AsynchronousGA
 *
 *  Created by Bill Sellers on 19/10/2010.
 *  Copyright 2010 Bill Sellers. All rights reserved.
 *
 */

#include "Preferences.h"
#include "Genome.h"
#include "Population.h"
#include "Mating.h"
#include "Random.h"
#include "Statistics.h"
#include "GAASIO.h"
#include "MD5.h"
#include "ServerASIO.h"
#include "ArgParse.h"

#include "pystring.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <map>
#include <algorithm>
#include <cstdarg>
#include <thread>
#include <cinttypes>
#include <filesystem>
#include <regex>
#if ! ( defined(WIN32) || defined(_WIN32))
#include <sys/resource.h>
#endif

using namespace std::string_literals;

class CloseFileGuard
{
public:
    CloseFileGuard(std::ofstream *file) { m_file = file; }
    ~CloseFileGuard() { if (m_file) m_file->close(); }
private:
    std::ofstream *m_file = nullptr;
};

int main(int argc, const char **argv)
{
    std::string compileDate(__DATE__);
    std::string compileTime(__TIME__);
    ArgParse argparse;
    argparse.Initialise(argc, argv, "AsynchronousGA2022 distributed genetic algorithm program "s + compileDate + " "s + compileTime, 0, 0);
    // required arguments
    argparse.AddArgument("-p"s, "--parameterFile"s, "Parameter file specifying the GA options"s, ""s, 1, true, ArgParse::String);
    argparse.AddArgument("-b"s, "--baseXMLFile"s, "Base XML file that is optimised"s, ""s, 1, true, ArgParse::String);
    argparse.AddArgument("-s"s, "--startingPopulation"s, "Starting population"s, ""s, 1, true, ArgParse::String);
    argparse.AddArgument("-t"s, "--serverPort"s, "The server TCP port to listen on"s, ""s, 1, true, ArgParse::Int);
    // optional arguments
    argparse.AddArgument("-o"s, "--outputDirectory"s, "Output directory [uses current date & time]"s, ""s, 1, false, ArgParse::String);
    argparse.AddArgument("-l"s, "--logLevel"s, "0, 1, 2 outputs more detail with higher numbers [0]"s, "0"s, 1, false, ArgParse::Int);

    int err = argparse.Parse();
    if (err)
    {
        argparse.Usage();
        exit(1);
    }

    int logLevel, serverPort;
    std::string baseXMLFile, parameterFile, outputDirectory, startingPopulation;
    argparse.Get("--logLevel"s, &logLevel);
    argparse.Get("--serverPort"s, &serverPort);
    argparse.Get("--baseXMLFile"s, &baseXMLFile);
    argparse.Get("--parameterFile"s, &parameterFile);
    argparse.Get("--outputDirectory"s, &outputDirectory);
    argparse.Get("--startingPopulation"s, &startingPopulation);
    
    GAMain ga;
    ga.SetLogLevel(logLevel);
    ga.LoadBaseXMLFile(baseXMLFile);
    ga.SetServerPort(serverPort);
    return ga.Process(parameterFile, outputDirectory, startingPopulation);
}

GAMain::GAMain()
{
    m_outputLogFile.exceptions(std::ios::failbit|std::ios::badbit);
}

int GAMain::Process(const std::string &parameterFile, const std::string &outputDirectory, const std::string &startingPopulation)

{
    // check and if necessary set some file open limits since this program can open a lot of files
#if defined(WIN32) || defined(_WIN32)
    ReportProgress("Original number of open files limit = "s + std::to_string(_getmaxstdio()), 1);
    const int windowsMaxNumberOfOpentFiles = 8096; // this is the current windows limit as defined in the _setmaxstdio documentation
    if (_getmaxstdio() < windowsMaxNumberOfOpentFiles)
    {
        int status = _setmaxstdio(windowsMaxNumberOfOpentFiles);
        if (status == -1) ReportProgress("Error calling _setmaxstdio(windowsMaxNumberOfOpentFiles)"s, 0);
        else ReportProgress("New number of open files limit = "s + std::to_string(_getmaxstdio()), 1);
    }
#else
    struct rlimit rlim;
    int status = getrlimit(RLIMIT_NOFILE, &rlim);
    if (status == -1)
    {
        ReportProgress("Error calling getrlimit(RLIMIT_NOFILE, &rlim)"s, 0);
    }
    else
    {
        ReportProgress("Original number of open files limits: soft = "s + std::to_string(rlim.rlim_cur) + " hard = "s + std::to_string(rlim.rlim_max), 1);
        if (rlim.rlim_cur < rlim.rlim_max)
        {
            rlim.rlim_cur = rlim.rlim_max;
            status = setrlimit(RLIMIT_NOFILE, &rlim);
            if (status == -1) ReportProgress("Error calling setrlimit(RLIMIT_NOFILE, &rlim)"s, 0);
            else ReportProgress("New number of open files limits: soft = "s + std::to_string(rlim.rlim_cur) + " hard = "s + std::to_string(rlim.rlim_max), 1);
        }
    }
#endif

    time_t theTime = time(nullptr);
    struct tm *theLocalTime = localtime(&theTime);
    std::string logFileName;

    if (m_baseXMLFile.GetSize() == 0)
    {
        ReportProgress("Error: XML base file missing"s, 0);
        return __LINE__;
    }

    m_parameterFile = pystring::os::path::abspath(parameterFile, std::filesystem::current_path().u8string());
    if (m_preferences.ReadPreferences(m_parameterFile))
    {
        ReportProgress("Error reading "s + m_parameterFile, 0);
        return (__LINE__);
    }
    ReportProgress(m_parameterFile + " read"s, 0);

    // sanity check some of the parameters
    if (m_preferences.parentsToKeep >= m_preferences.populationSize)
    {
        ReportProgress("Error: parentsToKeep must be lower than populationSize"s, 0);
        return __LINE__;
    }

    // overwrite starting population if defined
    if (startingPopulation.size()) m_preferences.startingPopulation = startingPopulation;
    m_preferences.startingPopulation = pystring::os::path::abspath(m_preferences.startingPopulation, std::filesystem::current_path().u8string());

    // create a directory for all the output
    if (outputDirectory.size())
    {
        m_outputFolderName = pystring::os::path::abspath(outputDirectory, std::filesystem::current_path().u8string());
    }
    else
    {
        std::string timedFolder = ToString("Run_%04d-%02d-%02d_%02d.%02d.%02d",
                                           theLocalTime->tm_year + 1900,
                                           theLocalTime->tm_mon + 1,
                                           theLocalTime->tm_mday,
                                           theLocalTime->tm_hour,
                                           theLocalTime->tm_min,
                                           theLocalTime->tm_sec);
        m_outputFolderName = pystring::os::path::abspath(timedFolder, std::filesystem::current_path().u8string());
    }
    if (std::filesystem::exists(m_outputFolderName))
    {
        if (!std::filesystem::is_directory(std::filesystem::status(m_outputFolderName)))
        {
            ReportProgress(ToString("Error \"%s\" exists and is not a folder", m_outputFolderName.c_str()), 0);
            return __LINE__;
        }
        ReportProgress(m_outputFolderName + " used"s, 0);
    }
    else
    {
        try
        {
            std::filesystem::create_directories(m_outputFolderName);
        }
        catch (std::exception& e)
        {
            ReportProgress("Error creating directory "s + m_outputFolderName, 0);
            ReportProgress(e.what(), 0);
        }
        catch (...)
        {
            ReportProgress("Error creating directory "s + m_outputFolderName, 0);
            return __LINE__;
        }
        ReportProgress(m_outputFolderName + " created"s, 0);
    }

    // write log
    logFileName = pystring::os::path::join(m_outputFolderName, "log.txt"s);
    m_outputLogFile.open(logFileName.c_str());
    if (m_outputLogFile.fail())
    {
        ReportProgress(ToString("Error opening \"%s\": %s", logFileName.c_str(), std::strerror(errno)), 0);
        return __LINE__;
    }
    CloseFileGuard closeFileGuard(&m_outputLogFile);
    m_outputLogFile << "GA build " << __DATE__ << " " << __TIME__ "\n";
    m_outputLogFile << "Log produced " << asctime(theLocalTime);
    m_outputLogFile << "parameterFile \"" << m_parameterFile << "\"\n";
    m_outputLogFile << m_preferences.GetPreferencesString() << "\n";
    m_outputLogFile.flush();
    ReportProgress(logFileName + " opened"s, 0);

    // initialise the population
    m_startPopulation.SetGlobalCircularMutation(m_preferences.circularMutation);
    m_startPopulation.SetResizeControl(m_preferences.resizeControl);
    m_startPopulation.SetSelectionType(m_preferences.parentSelection);
    m_startPopulation.SetParentsToKeep(m_preferences.parentsToKeep);
    m_startPopulation.SetGamma(m_preferences.gamma);
    m_startPopulation.SetMinimizeScore(m_preferences.minimizeScore);
    if (m_startPopulation.ReadPopulation(m_preferences.startingPopulation.c_str()))
    {
        ReportProgress("Error reading starting population: "s + m_preferences.startingPopulation, 0);
        return __LINE__;
    }
    ReportProgress(m_preferences.startingPopulation + " read"s, 0);

    if (m_startPopulation.GetPopulationSize() != m_preferences.populationSize)
    {
        ReportProgress("Info: Starting population size "s + std::to_string(m_startPopulation.GetPopulationSize()) + " does not match specified population size "s + std::to_string(m_preferences.populationSize) + " - using standard fixup"s, 0);
        m_startPopulation.ResizePopulation(m_preferences.populationSize, &m_random);
    }
    if (m_startPopulation.GetGenome(0)->GetGenomeLength() != m_preferences.genomeLength)
    {
        ReportProgress("Error: Starting population genome does not match specified genome length"s, 0);
        return __LINE__;
    }
    if (m_preferences.randomiseModel)
    {
        m_startPopulation.Randomise(&m_random);
    }

    m_evolvePopulation.SetGlobalCircularMutation(m_preferences.circularMutation);
    m_evolvePopulation.SetResizeControl(m_preferences.resizeControl);
    m_evolvePopulation.SetSelectionType(m_preferences.parentSelection);
    m_evolvePopulation.SetParentsToKeep(m_preferences.parentsToKeep);
    m_evolvePopulation.SetGamma(m_preferences.gamma);
    m_evolvePopulation.SetMinimizeScore(m_preferences.minimizeScore);

    if (Evolve())
    {
        ReportProgress("Error: Terminated due to Evolve failure"s, 0);
        return __LINE__;
    }

    return 0;
}

int GAMain::LoadBaseXMLFile(const std::string &filename)
{
    if (m_baseXMLFile.ReadFile(filename))
    {
        m_baseXMLFile.ClearData();
        std::fill(std::begin(m_md5), std::end(m_md5), 0);
        return 1;
    }
    const uint32_t *md5Array = md5(m_baseXMLFile.GetRawData(), int(m_baseXMLFile.GetSize()));
    for (size_t i = 0; i < m_md5.size(); i++) m_md5[i] = md5Array[i];
    return 0;
}

int GAMain::Evolve()
{
    // This is the asynchronous evolution loop
    double evolveStartTime = std::chrono::duration_cast<std::chrono::duration<double, std::chrono::seconds::period>>(std::chrono::system_clock::now().time_since_epoch()).count();
    m_evolveIdentifier = uint64_t(evolveStartTime);
    uint32_t submitCount = 0;
    uint32_t returnCount = 0;
    int startPopulationIndex = 0;
    size_t parent1Rank, parent2Rank;
    int mutationCount;
    Genome *parent1;
    Genome *parent2;
    TenPercentiles tenPercentiles;
    double bestFitness = m_preferences.minimizeScore ? std::numeric_limits<double>::max(): -std::numeric_limits<double>::max();
    double lastBestFitness = m_preferences.minimizeScore ? std::numeric_limits<double>::max(): -std::numeric_limits<double>::max();
    std::string filename;
    bool stopSendingFlag = false;
    std::map<uint32_t, std::unique_ptr<RunSpecifier>> runningList;
    Mating mating(&m_random);
    bool shouldStop = false;

    ReportInfo(ToString("Evolve Identifier = %" PRIu64, m_evolveIdentifier));

    // start the TCP server
    ServerASIO *server = new ServerASIO();
    if (server->setPort(uint16_t(m_tcpPort)))
    {
        ReportProgress(ToString("Unable to set listening port to %d", m_tcpPort), 0);
        delete server;
        return __LINE__;
    }
    server->attach("req_gen_"s, std::bind(&GAMain::handleRequestGenome, this, std::placeholders::_1));
    server->attach("req_xml_"s, std::bind(&GAMain::handleRequestXML, this, std::placeholders::_1));
    server->attach("score___"s, std::bind(&GAMain::handleScore, this, std::placeholders::_1));
    std::thread *serverThread = new std::thread(&ServerASIO::start, server);
    StopServerASIOGuard serverGuard(server, serverThread);
    m_requestGenomeQueueEnabled = true;
    server->getLocalAddress(&m_ipAddress, &m_port);

    int progressValue = 0;
    int lastProgressValue = -1;
    double lastTime = evolveStartTime;
    double lastSlowTime = evolveStartTime;
    double fastPeriodicTaskInterval = 0.1; // this is used for things like response to user interaction so 0.1s is about as high as it should be
    double slowPeriodicTaskInterval = 100; // this is used for internal housekeeping of things like the watchDogTimerLimit so 100s should be fine
    while (returnCount < uint32_t(m_preferences.maxReproductions) && stopSendingFlag == false && shouldStop == false)
    {
        double currentTime = std::chrono::duration_cast<std::chrono::duration<double, std::chrono::seconds::period>>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (currentTime >= lastTime + fastPeriodicTaskInterval) // this part of the loop is for things that don't need to be done all that often
        {
            lastTime = currentTime;
            if (pollStdin())
            {
                std::string instruction;
                std::getline(std::cin, instruction);
                instruction = pystring::strip(instruction);
                if (instruction == "stop"s)
                {
                    shouldStop = true;
                    ReportProgress("Stopped by user"s, 0);
                }
                if (instruction.rfind("log"s, 0) == 0)
                {
                    m_logLevel = std::atoi(instruction.c_str() + 3); // used std::atoi rather that std::stoi because std::atoi does not throw exceptions
                    ReportProgress(ToString("Log level changed to %d", m_logLevel), 0);
                }
            }
            progressValue = int(100 * returnCount / m_preferences.maxReproductions);
            if (progressValue != lastProgressValue)
            {
                lastProgressValue = progressValue;
                ReportInfo(ToString("Progress = %d", progressValue));
            }
        }
        if (currentTime >= lastSlowTime + slowPeriodicTaskInterval) // this part of the loop is for things that don't need to be done all that often
        {
            lastSlowTime = currentTime;
            for (auto &&it = runningList.begin(); it != runningList.end();)
            {
                if (currentTime - it->second->startTime > m_preferences.watchDogTimerLimit)
                {
                    ReportProgress(ToString("RunID %" PRIu32 " has been deleted", it->first), 2);
                    it = runningList.erase(it); // erase invalidates the iterator but returns the next valid iterator
                }
                else { it++; }
            }
        }

        size_t genomeQueueSize = GenomeRequestQueueSize();
        if (genomeQueueSize)
        {
            MessageASIO message;
            GetNextGenomeRequest(&message);
            const RequestMessage *messageContent = reinterpret_cast<const RequestMessage *>(message.content.data());
            Genome offspring;
            // if we are still working from the start population, just get the next one
            if (startPopulationIndex < m_startPopulation.GetPopulationSize())
            {
                offspring = *m_startPopulation.GetGenome(startPopulationIndex);
                startPopulationIndex++;
            }
            else
            {
                // create a new offspring
                mutationCount = 0;
                while (mutationCount == 0) // this means we always get some mutation (no point in getting unmutated offspring)
                {
                    if (m_evolvePopulation.GetPopulationSize()) parent1 = m_evolvePopulation.ChooseParent(&parent1Rank, &m_random);
                    else parent1 = m_startPopulation.ChooseParent(&parent1Rank, &m_random);
                    if (m_random.CoinFlip(m_preferences.crossoverChance))
                    {
                        if (m_evolvePopulation.GetPopulationSize()) parent2 = m_evolvePopulation.ChooseParent(&parent2Rank, &m_random);
                        else parent2 = m_startPopulation.ChooseParent(&parent2Rank, &m_random);
                        offspring = *parent1;
                        mutationCount += mating.Mate(parent1, parent2, &offspring, m_preferences.crossoverType);
                    }
                    else
                    {
                        offspring = *parent1;
                    }
                    if (m_preferences.multipleGaussian)  mutationCount += mating.MultipleGaussianMutate(&offspring, m_preferences.gaussianMutationChance, m_preferences.bounceMutation);
                    else mutationCount += mating.GaussianMutate(&offspring, m_preferences.gaussianMutationChance, m_preferences.bounceMutation);

                    mutationCount += mating.FrameShiftMutate(&offspring, m_preferences.frameShiftMutationChance);
                    mutationCount += mating.DuplicationMutate(&offspring, m_preferences.duplicationMutationChance);
                }
            }
            // got a genome to send
            std::vector<char> dataMessage(sizeof(DataMessage) + offspring.GetGenomeLength() * sizeof(double));
            DataMessage *dataMessagePtr = reinterpret_cast<DataMessage *>(dataMessage.data());
            strncpy(dataMessagePtr->text, "genome", sizeof(dataMessagePtr->text));
//            server.GetMyAddress(&dataMessagePtr->senderIP, &dataMessagePtr->senderPort);
            dataMessagePtr->evolveIdentifier = m_evolveIdentifier;
            dataMessagePtr->runID = submitCount;
            dataMessagePtr->genomeLength = uint32_t(offspring.GetGenomeLength());
            dataMessagePtr->xmlLength = uint32_t(m_baseXMLFile.GetSize());
            std::copy(std::begin(m_md5), std::end(m_md5), std::begin(dataMessagePtr->md5));
            std::copy_n(offspring.GetGenes()->data(), offspring.GetGenomeLength(), dataMessagePtr->payload.genome);
            auto sharedPtr = message.session.lock();
            if (sharedPtr)
            {
                sharedPtr->write(dataMessage.data(), dataMessage.size());
                std::unique_ptr<RunSpecifier> runSpecifier = std::make_unique<RunSpecifier>();
                runSpecifier->genome = std::move(offspring);
                runSpecifier->startTime = currentTime;
                runSpecifier->senderPort = messageContent->senderPort;
                runSpecifier->senderIP = messageContent->senderIP;
                runningList[submitCount] = std::move(runSpecifier);
                std::string address = ConvertAddressPortToString(messageContent->senderIP, uint16_t(messageContent->senderPort));
                ReportProgress(ToString("Sample %" PRIu32 " [%zu bytes] sent to %s evolveIdentifier %" PRIu64, submitCount, dataMessage.size(), address.c_str(), m_evolveIdentifier), 2);
                submitCount++;
            }
            else
            {
                ReportProgress(ToString("Sample %" PRIu32 " evolveIdentifier %" PRIu64 " unable to lock pointer", submitCount, m_evolveIdentifier), 1);
            }
            continue;
        }

        size_t scoreQueueSize = ScoreQueueSize();
        if (scoreQueueSize)
        {
            MessageASIO message;
            GetNextScore(&message);
            const RequestMessage *messageContent = reinterpret_cast<const RequestMessage *>(message.content.data());
            if (returnCount % 100 == 0) ReportInfo(ToString("Return Count = %" PRIu32, returnCount));
            uint32_t index = messageContent->runID;
            double result = messageContent->score;
            std::string address = ConvertAddressPortToString(messageContent->senderIP, messageContent->senderPort);
            ReportProgress(ToString("Sample %" PRIu32 " score %g from %s evolveIdentifier %" PRIu64, index, result, address.c_str(), messageContent->evolveIdentifier), 2);
            auto iter = runningList.find(index);
            if (messageContent->evolveIdentifier != m_evolveIdentifier || iter == runningList.end())
            {
                ReportProgress(ToString("Sample %" PRIu32 " not found score %g from %s evolveIdentifier %" PRIu64, index, result, address.c_str(), messageContent->evolveIdentifier), 1);
                continue;
            }
            iter->second->genome.SetFitness(result);
            // std::cerr << iter->second->genome;
            m_evolvePopulation.InsertGenome(std::move(iter->second->genome), m_preferences.populationSize);
            runningList.erase(iter);

            if (returnCount % uint32_t(m_preferences.outputStatsEvery) == uint32_t(m_preferences.outputStatsEvery) - 1)
            {
                CalculateTenPercentiles(&m_evolvePopulation, &tenPercentiles);
                m_outputLogFile << std::setw(10) << returnCount << " ";
                m_outputLogFile << tenPercentiles << "\n";
                m_outputLogFile.flush();
            }

            if (returnCount % uint32_t(m_preferences.saveBestEvery) == uint32_t(m_preferences.saveBestEvery) - 1 || returnCount == 1)
            {
                if ((m_preferences.minimizeScore && m_evolvePopulation.GetLastGenome()->GetFitness() < bestFitness) ||
                    (!m_preferences.minimizeScore && m_evolvePopulation.GetLastGenome()->GetFitness() > bestFitness))
                {
                    bestFitness = m_evolvePopulation.GetLastGenome()->GetFitness();
                    filename = pystring::os::path::join(m_outputFolderName, ToString(m_bestGenomeModel.c_str(), returnCount));
                    try
                    {
                        ReportProgress("Writing "s + filename, 1);
                        std::ofstream bestFile;
                        bestFile.exceptions (std::ios::failbit|std::ios::badbit);
                        bestFile.open(filename);
                        bestFile << *m_evolvePopulation.GetLastGenome();
                        bestFile.close();
                    }
                    catch (std::exception& e)
                    {
                        ReportProgress("Error writing "s + filename, 0);
                        ReportProgress(e.what(), 0);
                    }
                    catch (...)
                    {
                        ReportProgress("Error writing "s + filename, 0);
                    }
                    ReportInfo(ToString("Best Score = %g", bestFitness));
                }
            }

            if (returnCount % uint32_t(m_preferences.savePopEvery) == uint32_t(m_preferences.savePopEvery) - 1 || returnCount == 0)
            {
                filename = pystring::os::path::join(m_outputFolderName, ToString(m_bestPopulationModel.c_str(), returnCount));
                ReportProgress("Writing "s + filename, 1);
                int err = m_evolvePopulation.WritePopulation(filename.c_str(), m_preferences.outputPopulationSize);
                if (err) { ReportProgress("Error writing "s + filename, 0); }
            }

            if (returnCount % uint32_t(m_preferences.improvementReproductions) == uint32_t(m_preferences.improvementReproductions) - 1)
            {
                ReportProgress(ToString("Fitness change for %d reproductions is %g", m_preferences.improvementReproductions, std::abs(bestFitness - lastBestFitness)), 2);
                if (std::abs(bestFitness - lastBestFitness) < m_preferences.improvementThreshold ) stopSendingFlag = true; // it will now quit
                lastBestFitness = bestFitness;
            }

            returnCount++;
            continue;
        }

        if (!scoreQueueSize && !genomeQueueSize) { std::this_thread::sleep_for(std::chrono::microseconds(m_loopSleepTimeMicroSeconds)); }
    }

    if (returnCount) returnCount--; // reduce return count back to the value for the last actual return
    ReportProgress(ToString("GA evolveIdentifier = %" PRIu64 " ended returnCount = %" PRIu32 "", m_evolveIdentifier, returnCount), 1);

    if (m_evolvePopulation.GetPopulationSize())
    {
        if ((m_preferences.minimizeScore && m_evolvePopulation.GetLastGenome()->GetFitness() < bestFitness) ||
            (!m_preferences.minimizeScore && m_evolvePopulation.GetLastGenome()->GetFitness() > bestFitness))
        {
            filename = pystring::os::path::join(m_outputFolderName, ToString(m_bestGenomeModel.c_str(), returnCount));
            if (!std::filesystem::exists(filename))
            {
                try
                {
                    ReportProgress("Writing final "s + filename, 1);
                    std::ofstream bestFile(filename);
                    bestFile << *m_evolvePopulation.GetLastGenome();
                }
                catch (std::exception& e)
                {
                    ReportProgress("Error writing "s + filename, 0);
                    ReportProgress(e.what(), 0);
                }
                catch(...)
                {
                    ReportProgress("Error writing "s + filename, 0);
                }
            }
        }

        filename = pystring::os::path::join(m_outputFolderName, ToString(m_bestPopulationModel.c_str(), returnCount));
        if (!std::filesystem::exists(filename))
        {
            int err = m_evolvePopulation.WritePopulation(filename.c_str(), m_preferences.outputPopulationSize);
            if (err) { ReportProgress("Error writing "s + filename, 0); }
        }

        if (m_preferences.onlyKeepBestGenome) OnlyKeepLastMatching(m_bestGenomeRegex);
        if (m_preferences.onlyKeepBestPopulation) OnlyKeepLastMatching(m_bestPopulationRegex);
    }

    m_requestGenomeQueueEnabled = false;
    ClearGenomeRequestQueue();
    ClearScoreQueue();

    return 0;
}

void GAMain::SetServerPort(int port)
{
    m_tcpPort = port;
}

std::string GAMain::ConvertAddressPortToString(uint32_t address, uint16_t port)
{
    std::string hostURL;
    enum
    {
        O32_LITTLE_ENDIAN = 0x03020100ul,
        O32_BIG_ENDIAN = 0x00010203ul,
        O32_PDP_ENDIAN = 0x01000302ul,      /* DEC PDP-11 (aka ENDIAN_LITTLE_WORD) */
        O32_HONEYWELL_ENDIAN = 0x02030001ul /* Honeywell 316 (aka ENDIAN_BIG_WORD) */
    };
    const union { unsigned char bytes[4]; uint32_t value; } o32_host_order = { { 0, 1, 2, 3 } };

    if (o32_host_order.value == O32_BIG_ENDIAN)
    {
        hostURL = (std::to_string(address & 0xff) + "."s +
                   std::to_string((address >> 8) & 0xff) + "."s +
                   std::to_string((address >> 16) & 0xff) + "."s +
                   std::to_string(address >> 24) +":"s +
                   std::to_string(port));
    }
    if (o32_host_order.value == O32_LITTLE_ENDIAN)
    {
        hostURL = (std::to_string(address >> 24) +":"s +
                   std::to_string((address >> 16) & 0xff) + "."s +
                   std::to_string((address >> 8) & 0xff) + "."s +
                   std::to_string(address & 0xff) + "."s +
                   std::to_string(port));
    }
    return hostURL;
}

std::string GAMain::ConvertAddressToString(uint32_t address)
{
    std::string hostURL;
    enum
    {
        O32_LITTLE_ENDIAN = 0x03020100ul,
        O32_BIG_ENDIAN = 0x00010203ul,
        O32_PDP_ENDIAN = 0x01000302ul,      /* DEC PDP-11 (aka ENDIAN_LITTLE_WORD) */
        O32_HONEYWELL_ENDIAN = 0x02030001ul /* Honeywell 316 (aka ENDIAN_BIG_WORD) */
    };
    const union { unsigned char bytes[4]; uint32_t value; } o32_host_order = { { 0, 1, 2, 3 } };
    if (o32_host_order.value == O32_BIG_ENDIAN)
    {
        hostURL = (std::to_string(address & 0xff) + "."s +
                   std::to_string((address >> 8) & 0xff) + "."s +
                   std::to_string((address >> 16) & 0xff) + "."s +
                   std::to_string(address >> 24));
    }
    if (o32_host_order.value == O32_LITTLE_ENDIAN)
    {
        hostURL = (std::to_string(address >> 24) +":"s +
                   std::to_string((address >> 16) & 0xff) + "."s +
                   std::to_string((address >> 8) & 0xff) + "."s +
                   std::to_string(address & 0xff) + "."s);
    }
    return hostURL;
}

void GAMain::ReportProgress(const std::string &message, int logLevel)
{
    if (m_logLevel >= logLevel)
    {
        std::cout << message << "\n";
        std::cout.flush();
    }
}

void GAMain::ReportInfo(const std::string &message)
{
    std::cerr << message << "\n";
    std::cerr.flush();
}

// convert to string using printf style formatting and variable numbers of arguments
std::string GAMain::ToString(const char * const printfFormatString, ...)
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

void GAMain::handleRequestGenome(MessageASIO message)
{
    if (message.content.size() < sizeof(RequestMessage)) return;
    if (!m_requestGenomeQueueEnabled) return;
    if (auto sharedPtr1 = message.session.lock())
    {
        std::unique_lock<std::mutex> lock(m_requestGenomeMutex);
        for (auto &&it : m_requestGenomeQueue)
        {
            // check to see whether we already have a genome request from this session
            if (auto sharedPtr2 = it.session.lock())
            {
                if (sharedPtr1.get() == sharedPtr2.get()) return;
            }
        }
        m_requestGenomeQueue.push_back(message);
    }
}

void GAMain::handleRequestXML(MessageASIO message)
{
    if (message.content.size() < sizeof(RequestMessage)) return;
    const RequestMessage *messageContent = reinterpret_cast<const RequestMessage *>(message.content.data());
    std::vector<char> dataMessage(sizeof(DataMessage) + m_baseXMLFile.GetSize() * sizeof(char));
    DataMessage *dataMessagePtr = reinterpret_cast<DataMessage *>(dataMessage.data());
    strncpy(dataMessagePtr->text, "xml", 16);
    dataMessagePtr->senderIP = m_ipAddress[0] * 0xffffff + m_ipAddress[1] * 0xffff + m_ipAddress[2] * 0xff + m_ipAddress[3];
    dataMessagePtr->senderPort = m_port;
    dataMessagePtr->runID = std::numeric_limits<uint32_t>::max();
    dataMessagePtr->evolveIdentifier = m_evolveIdentifier;
    dataMessagePtr->genomeLength = uint32_t(m_startPopulation.GetFirstGenome()->GetGenomeLength());
    dataMessagePtr->xmlLength = uint32_t(m_baseXMLFile.GetSize());
    std::copy(std::begin(m_md5), std::end(m_md5), std::begin(dataMessagePtr->md5));
    std::copy_n(m_baseXMLFile.GetRawData(), m_baseXMLFile.GetSize(), dataMessagePtr->payload.xml);
    if (auto sharedPtr = message.session.lock())
        sharedPtr->write(dataMessage.data(), dataMessage.size());
    std::string address = ConvertAddressPortToString(messageContent->senderIP, messageContent->senderPort);
    ReportProgress(ToString("XML %zu bytes sent to %s", int(dataMessage.size()), address.c_str()), 2);
}

int GAMain::OnlyKeepLastMatching(const std::string &regexPattern)
{
    const std::regex regex(regexPattern);
    std::vector<std::string> directoryContents;
    try
    {
        for (auto &&entry : std::filesystem::directory_iterator(m_outputFolderName))
        {
            std::string basename = pystring::os::path::basename(entry.path().u8string());
            if (std::regex_match(basename, regex)) { directoryContents.push_back(entry.path().u8string()); }
        }
    }
    catch (std::exception& e)
    {
        ReportProgress("Error reading output directory "s + m_outputFolderName, 0);
        ReportProgress(e.what(), 0);
    }
    catch (...)
    {
        ReportProgress("Error reading output directory "s + m_outputFolderName, 0);
        return __LINE__;
    }
    if (directoryContents.size() == 0)
    {
        ReportProgress(ToString("Error: could not find \"%s\" in \"%s\"", regexPattern.c_str(), m_outputFolderName.c_str()), 0);
        return __LINE__;
    }
    std::sort(directoryContents.begin(), directoryContents.end());
    for (size_t i = 0; i < directoryContents.size() - 1; i++) // loop to all but last entry
    {
        try
        {
            ReportProgress("Removing "s + directoryContents[i], 1);
            std::filesystem::remove(directoryContents[i]);
        }
        catch (std::exception& e)
        {
            ReportProgress("Error deleting "s + directoryContents[i], 0);
            ReportProgress(e.what(), 0);
        }
        catch (...)
        {
            ReportProgress("Error deleting "s + directoryContents[i], 0);
            return __LINE__;
        }
    }
    return 0;
}

void GAMain::handleScore(MessageASIO message)
{
    if (message.content.size() < sizeof(RequestMessage)) return;
    std::unique_lock<std::mutex> lock(m_scoreMutex);
    m_scoreQueue.push_back(message);
}

size_t GAMain::GenomeRequestQueueSize()
{
    std::unique_lock<std::mutex> lock(m_requestGenomeMutex);
    return (m_requestGenomeQueue.size());
}

size_t GAMain::ScoreQueueSize()
{
    std::unique_lock<std::mutex> lock(m_scoreMutex);
    return (m_scoreQueue.size());
}

void GAMain::GetNextGenomeRequest(MessageASIO *message)
{
    std::unique_lock<std::mutex> lock(m_requestGenomeMutex);
    *message = m_requestGenomeQueue.front();
    m_requestGenomeQueue.pop_front();
}

void GAMain::GetNextScore(MessageASIO *message)
{
    std::unique_lock<std::mutex> lock(m_scoreMutex);
    *message = m_scoreQueue.front();
    m_scoreQueue.pop_front();
}

void GAMain::ClearGenomeRequestQueue()
{
    std::unique_lock<std::mutex> lock(m_requestGenomeMutex);
    m_requestGenomeQueue.clear();
}

void GAMain::ClearScoreQueue()
{
    std::unique_lock<std::mutex> lock(m_scoreMutex);
    m_scoreQueue.clear();
}

// returns true if characters are available to read from stdin
bool GAMain::pollStdin()
{
#if defined(WIN32) || defined(_WIN32)
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    // Windows uses different functions depending on whether the console has been redirected
    // if it hasn't then we would probably need to use PeekConsoleInput and various other tricks
    DWORD fileType = GetFileType(h);
    //std::cout << "GetFileType " << fileType << "\n";
    if (fileType != FILE_TYPE_PIPE) return false; // this means that we are not running this as a subprocess with a pipe
//    char buffer[64];
//    DWORD nBufferSize = sizeof(buffer);
//    DWORD nBytesRead = 0, nTotalBytesAvail = 0, nBytesLeftThisMessage = 0;
//    BOOL successFlag = PeekNamedPipe(h, &buffer, nBufferSize, &nBytesRead, &nTotalBytesAvail, &nBytesLeftThisMessage); // does look like I need to peek some bytes for this to work properly
//    //std::cout << "PeekNamedPipe successFlag " << successFlag << " h " << h << " buffer " << buffer << " nBytesRead " << nBytesRead << " nTotalBytesAvail " << nTotalBytesAvail << " nBytesLeftThisMessage " << nBytesLeftThisMessage << "\n";
//    //std::cout.flush();
//    if (successFlag && nBytesRead) return true;
    DWORD nTotalBytesAvail = 0;
    BOOL successFlag = PeekNamedPipe(h, 0, 0, 0, &nTotalBytesAvail, 0);
    if (successFlag && nTotalBytesAvail) return true;
    return false;
#else
    pollfd cinfd[1];
    cinfd[0].fd = fileno(stdin);
    cinfd[0].events = POLLIN;
    if (poll(cinfd, 1, 0)) return true;
    return false;
#endif
}




