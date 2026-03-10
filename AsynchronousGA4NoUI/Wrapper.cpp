#include "Wrapper.h"

#include "MergeUtil.h"
#include "MergeXML.h"
#include "XMLConverter.h"

#include "pystring.h"

#include <filesystem>
#include <cstdio>

#if defined(_WIN32)
#include <algorithm>
#else
#include <unistd.h>
#endif



using namespace std::string_literals;

Wrapper::Wrapper()
{
}

void Wrapper::runGA()
{
    std::vector<std::string> arguments{"--parameterFile", m_parameterFile,
                                       "--baseXMLFile", m_xmlMasterFile,
                                       "--startingPopulation", m_startingPopulationFile,
                                       "--outputDirectory", m_outputFolder,
                                       "--serverPort", std::to_string(m_serverPort),
                                       "--logLevel", std::to_string(m_logLevel)};
    int status;
    std::string output = runCommand(m_gaExecutable, arguments, &status);
    std::cerr << output;
    if (status)
    {
        std::cerr << "Error " << status << " running: " << m_gaExecutable << pystring::join(" "s, arguments) << "\n";
        std::exit(status);
    }
}

void Wrapper::runMergeXML()
{
    std::cerr << "Running MergeXML\n";
    // get the population and config from the models
    std::filesystem::path lastPopulation = m_modelPopulationFile;
    std::filesystem::path lastConfig = m_modelConfigurationFile;
    m_currentLoopValue = m_startValue;
    m_currentLoopCount = 0;

    while (true)
    {
        if ((m_stepValue >= 0 && m_currentLoopValue >= m_endValue) || (m_stepValue < 0 && m_currentLoopValue <= m_endValue))
        {
            std::cerr << "Finished loop\n";
            std::exit(0);
        }

        if (m_currentLoopCount)
        {
            // get the population and config from the output of the previous runGA
            runGaitSym(); // run gaitsym and generate ModelState.xml
            std::filesystem::path outputFolder(m_outputFolder);
            lastConfig = (outputFolder / "ModelState.xml");
            if (!std::filesystem::exists(lastConfig))
            {
                std::cerr << "Error: MergeXML: Unable to find ModelState.xml in: \"" << m_outputFolder << "\"\n";
                exit(1);
            }
            std::vector<std::filesystem::path> files = listFilesMatching(outputFolder, std::regex("^Population.*.txt$"));
            if (files.size() == 0)
            {
                std::cerr << "Error: MergeXML: Unable to find Population*.txt in: \"" << m_outputFolder << "\"\n";
                exit(1);
            }

            lastPopulation = files.back();
            m_currentLoopCount++;
            m_currentLoopValue += m_stepValue;
            if (m_stepValue >= 0 && (m_currentLoopValue > m_endValue || fabs(m_currentLoopValue - m_endValue) < fabs(m_stepValue) * 0.0001)) m_currentLoopValue = m_endValue;
            if (m_stepValue < 0 && (m_currentLoopValue < m_endValue || fabs(m_currentLoopValue - m_endValue) < fabs(m_stepValue) * 0.0001)) m_currentLoopValue = m_endValue;
        }


        std::filesystem::path driverFile(m_driverFile);
        std::filesystem::path workingFolder(m_workingFolder);
        std::string errorMessage;
        std::string mergeXMLCommands = readFile(m_mergeXMLFile, &errorMessage);
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

        std::filesystem::path subFolder(toString("%04d_Run_%s", timeString.c_str()));
        std::filesystem::path outputFolder = workingFolder / subFolder;
        m_outputFolder = outputFolder.string();
        std::filesystem::create_directories(outputFolder);
        if (!std::filesystem::is_directory(outputFolder))
        {
            std::cerr << "Error: MergeXML: Unable to create folder: \"" << outputFolder << "\"\n";
            std::exit(1);
        }
        std::filesystem::path workingConfig = (outputFolder / "workingConfig.xml");
        m_xmlMasterFile = workingConfig.string();
        std::filesystem::path firstConfigFile = m_modelConfigurationFile;
        std::string replace = pystring::replace(mergeXMLCommands, "MODEL_CONFIG_FILE", lastConfig.string());
        replace = pystring::replace(replace, "DRIVER_CONFIG_FILE", driverFile.string());
        replace = pystring::replace(replace, "WORKING_CONFIG_FILE", workingConfig.string());
        replace = pystring::replace(replace, "CURRENT_LOOP_VALUE", toString("%.*gs", 17, m_currentLoopValue));
        replace = pystring::replace(replace, "FIRST_CONFIG_FILE", firstConfigFile.string());
        MergeXML mergeXML;
        mergeXML.ExecuteInstructionFile(replace);
        if (mergeXML.errorMessageList().size())
        {
            std::cerr << "Error: MergeXML: Unable to parse file: \"" << m_mergeXMLFile << "\"\n";
            std::exit(1);
        }
        std::filesystem::path newMergeXML = outputFolder / "workingMergeXML.txt";
        std::ofstream file(newMergeXML, std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Error: MergeXML: Open File Error: \"" << newMergeXML << "\" " << std::error_code(errno, std::system_category()).message() << "\n";
            std::exit(1);
        }
        file.write(replace.c_str(), replace.size());
        file.close();
        std::filesystem::path mergeXMLStatusFile = outputFolder / "mergeXMLStatus.txt";
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
        if (m_cycleFlag) stream << "outputCycle " << m_outputCycle << "\n";
        else stream << "outputTime " << m_outputCycle << "\n";
        stream << "CURRENT_LOOP_VALUE " << m_currentLoopValue << "\n";
        stream << "MODEL_CONFIG_FILE " << lastConfig << "\n";
        stream << "DRIVER_CONFIG_FILE " << driverFile << "\n";
        stream << "WORKING_CONFIG_FILE " << workingConfig << "\n";
        stream << "FIRST_CONFIG_FILE " << firstConfigFile << "\n";
        stream.close();
        std::filesystem::path newPopulation = outputFolder / "workingPopulation.txt";
        std::filesystem::copy_file(lastPopulation, newPopulation);
        m_startingPopulationFile = newPopulation.string();
        runPostMergeScript();
        runGA();
    }
}

void Wrapper::runPostMergeScript()
{
    if (m_postMergeScript.empty()) return;
    std::filesystem::path mergeScriptExecutable = m_postMergeScript;

    std::string interpreter;
    if (isExecutableFile(mergeScriptExecutable))
    {
        std::vector<std::string> arguments << "--startingPopulationFile" << ui->lineEditStartingPopulationFile->text()
                  << "--xmlMasterFile" << ui->lineEditXMLMasterFile->text()
                  << "--outputFolder" << ui->lineEditOutputFolder->text()
                  << "--currentLoopValue" << std::string::number(m_currentLoopValue, 'g', 17)
                  << "--logLevel" << ui->spinBoxLogLevel->text();
        mergeScript.start(mergeScriptExecutable, arguments);
    }
    else
    {
        if (mergeScriptExecutable.endsWith(".py", Qt::CaseInsensitive))
        {
            while (interpreter.isEmpty())
            {
                interpreter = existsOnPath("python");
                if (interpreter.size()) break;
                interpreter = existsOnPath("python3");
                if (interpreter.size()) break;
                std::stringList options{"C:/ProgramData/miniconda3/python.exe", "C:/ProgramData/anaconda3/python.exe", "C:/Program Files/Spyder/Python/python.exe"};
                for (auto &&option : options)
                {
                    QFileInfo info(option);
                    if (info.exists() && info.isExecutable()) { interpreter = option; break; }
                }
                break;
            }
        }
        if (mergeScriptExecutable.endsWith(".r", Qt::CaseInsensitive))
        {
            interpreter = existsOnPath("Rscript");
        }
        if (interpreter.isEmpty())
        {
            tryToStopGA();
            m_mergeXMLTimer->stop();
            m_runMergeXML = 0;
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("runPostMergeScript: Unknown script \"%1\"").arg(mergeScriptExecutable));
            QMessageBox::warning(this, tr("Run Post Merge Script Error"), std::string("runPostMergeScript: Unknown script \"%1\"").arg(mergeScriptExecutable));
            return;
        }
        arguments << mergeScriptExecutable
                  << "--startingPopulationFile" << ui->lineEditStartingPopulationFile->text()
                  << "--xmlMasterFile" << ui->lineEditXMLMasterFile->text()
                  << "--outputFolder" << ui->lineEditOutputFolder->text()
                  << "--currentLoopValue" << std::string::number(m_currentLoopValue, 'g', 17)
                  << "--logLevel" << ui->spinBoxLogLevel->text();
        mergeScript.start(interpreter, arguments);
    }

    int msecs = 60 * 60 * 1000; // FIX ME - this should be user settable
    if (!mergeScript.waitForFinished(msecs)) // this should work although if mergeScript finished really quickly it is not guaranteed
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("runPostMergeScript: Timeout \"%1\"").arg(mergeScriptExecutable));
        QMessageBox::warning(this, tr("Run Post Merge Script Error"), std::string("runPostMergeScript: Timeout \"%1\"").arg(mergeScriptExecutable));
        return;
    }
    QByteArray stdOut = mergeScript.readAllStandardOutput();
    appendProgress(std::string::fromStdString(stdOut.toStdString()));
    QByteArray stdError = mergeScript.readAllStandardError();
    appendProgress(std::string::fromStdString(stdError.toStdString()));
}

void Wrapper::runGaitSym()
{
    std::cerr << "Running GaitSym\n";
    std::filesystem::path dir(m_outputFolder);
    std::vector<std::filesystem::path> files = listFilesMatching(dir, std::regex("^BestGenome*.txt$"));
    if (files.size() < 1)
    {
        std::cerr << "Error: runGaitSym: Unable to find BestGenome*.txt\n";
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
    MergeUtil::readGenome(inputGenome.string(), &genes, &lowBounds, &highBounds, &gaussianSDs, &circularMutationFlags, &fitness, &genomeType);
    XMLConverter xmlConverter;
    xmlConverter.LoadBaseXMLFile(inputXML.string());
    xmlConverter.ApplyGenome(genes);
    std::string formattedXML;
    xmlConverter.GetFormattedXML(&formattedXML);
    std::ofstream outputXMLFile(outputXML, std::ios::binary);
    outputXMLFile.write(formattedXML.data(), formattedXML.size());
    outputXMLFile.close();

    std::filesystem::path modelStateFileName = dir / "ModelState.xml";
    std::filesystem::path gaitSymExecutable(m_gaitSymExecutable);
    if (!isExecutableFile(gaitSymExecutable))
    {
        std::cerr << "Error: runGaitSym: Unable to run \"" << std::filesystem::absolute(gaitSymExecutable) << "\"\n";
        std::exit(1);
    }
    std::vector<std::string> arguments;
    arguments.push_back("--config");
    arguments.push_back(outputXML.string());
    if (m_cycleFlag) arguments.push_back("--outputModelStateAtCycle");
    else arguments.push_back("--outputModelStateAtTime");
    arguments.push_back(toString("%.*g", 17, m_outputCycle)); // this lets you set the precision as an argument
    arguments.push_back("--modelState");
    arguments.push_back(modelStateFileName.string());
    int exitStatus;
    std::string output = runCommand(m_gaitSymExecutable, arguments, &exitStatus);
    std::cerr << output;
    if (exitStatus)
    {
        std::cerr << "Error: runGaitSym: fail running \"" << gaitSymExecutable << "\"\n";
        return;
    }
    if (!std::filesystem::exists(modelStateFileName) && !std::filesystem::is_regular_file(modelStateFileName))
    {
        std::cerr << "Error :runGaitSym: Unable to create: \"" << modelStateFileName << "\"\n";
        exit(1);
    }
}

void Wrapper::pushButtonStopClicked()
{
    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Stop Button Clicked - trying to abort GA");
    ui->statusBar->showMessage("Stop Button Clicked - trying to abort GA");
    tryToStopGA();
    m_mergeXMLTimer->stop();
    m_runMergeXML = 0;
    activateButtons();
}

void Wrapper::handleFinished()
{
    if (!m_ga)
    {
        appendProgress("Error in handleFinished");
        return;
    }
    int result = m_ga->exitCode();
    int exitStatus = m_ga->exitStatus();
    appendProgress(std::string::fromStdString(MergeUtil::toString("handleFinished exitCode = %d exitStatus = %d", result, exitStatus)));
    delete m_ga;
    m_ga = nullptr;
    if (result == 0 && exitStatus == 0)
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress("GA finished");
        ui->statusBar->showMessage("GA finished");
        ui->progressBar->setValue(0);
    }
    else
    {
        appendProgress("Error after running GA");
        ui->statusBar->showMessage("Error after running GA");
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
    }
    activateButtons();
}

void Wrapper::lineEditOutputFolderTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditParameterFileTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditXMLMasterFileTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditStartingPopulationFileTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditModelConfigurationFileTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditModelPopulationFileTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditDriverFileTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditMergeXMLFileTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditWorkingFolderTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditGaitSymExecutableTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditPostMergeScriptTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::lineEditGAExecutableTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::spinBoxLogLevelTextChanged(const std::string & /*text*/)
{
#if (!defined(WIN32) && ! defined(_WIN32)) || true
    std::string instruction = MergeUtil::toString("log%d\n", ui->spinBoxLogLevel->value());
    if (m_ga) m_ga->write(instruction.c_str(), instruction.size());
#endif
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::spinBoxTextChanged(const std::string & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::checkBoxStateChanged(int /*state*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void Wrapper::activateButtons()
{
    ui->pushButtonParameterFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    ui->pushButtonStartingPopulationFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    ui->pushButtonXMLMasterFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    ui->pushButtonOutputFolder->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    ui->pushButtonModelConfigurationFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->pushButtonModelPopulationFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->pushButtonDriverFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->pushButtonWorkingFolder->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->pushButtonMergeXMLFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->pushButtonGaitSymExecutable->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->pushButtonPostMergeScript->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->pushButtonGAExecutable->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    ui->lineEditOutputFolder->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && !ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditParameterFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    ui->lineEditXMLMasterFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && !ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditStartingPopulationFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && !ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditModelConfigurationFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditModelPopulationFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditDriverFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditWorkingFolder->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditMergeXMLFile->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditGaitSymExecutable->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditPostMergeScript->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditGAExecutable->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    ui->lineEditCurrentLoopCount->setEnabled(ui->checkBoxMergeXMLActivate->isChecked());
    ui->lineEditCurrentLoopValue->setEnabled(ui->checkBoxMergeXMLActivate->isChecked());
    ui->spinBoxLogLevel->setEnabled(true);
    ui->spinBoxPort->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    ui->doubleSpinBoxStartValue->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->doubleSpinBoxStepValue->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->doubleSpinBoxEndValue->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->doubleSpinBoxOutputCycle->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->checkBoxCycleTime->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && ui->checkBoxMergeXMLActivate->isChecked());
    ui->pushButtonStop->setEnabled(m_ga || m_runMergeXML != 0);
    ui->checkBoxMergeXMLActivate->setEnabled(m_ga == nullptr && m_runMergeXML == 0);

    QList<QLabel *> labels = ui->groupBoxMergeXMLControls->findChildren<QLabel *>();
    for (auto &&it : labels) it->setEnabled(ui->checkBoxMergeXMLActivate->isChecked());

    if (ui->checkBoxMergeXMLActivate->isChecked())
    {
        bool gaExecutableValid = false;
        QFileInfo gaExecutableInfo(ui->lineEditGAExecutable->text());
        if (gaExecutableInfo.exists() && gaExecutableInfo.isFile() && gaExecutableInfo.isExecutable()) gaExecutableValid = true;
        bool gaitSymExecutableValid = false;
        QFileInfo gaitSymExecutableInfo(ui->lineEditGaitSymExecutable->text());
        if (gaitSymExecutableInfo.exists() && gaitSymExecutableInfo.isFile() && gaitSymExecutableInfo.isExecutable()) gaitSymExecutableValid = true;
        bool mergeScriptExecutableValid = true;
        if (!ui->lineEditGaitSymExecutable->text().isEmpty())
        {
            QFileInfo mergeScriptExecutableInfo(ui->lineEditGaitSymExecutable->text());
            if (!mergeScriptExecutableInfo.exists() || !mergeScriptExecutableInfo.isFile()) mergeScriptExecutableValid = false;
        }
        ui->pushButtonStart->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && !ui->lineEditModelConfigurationFile->text().isEmpty() &&
                                        !ui->lineEditModelPopulationFile->text().isEmpty() && !ui->lineEditDriverFile->text().isEmpty() &&
                                        !ui->lineEditMergeXMLFile->text().isEmpty() && gaExecutableValid && gaitSymExecutableValid && mergeScriptExecutableValid);
    }
    else
    {
        bool gaExecutableValid = false;
        QFileInfo gaExecutableInfo(ui->lineEditGAExecutable->text());
        if (gaExecutableInfo.exists() && gaExecutableInfo.isFile() && gaExecutableInfo.isExecutable()) gaExecutableValid = true;
        ui->pushButtonStart->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && !ui->lineEditParameterFile->text().isEmpty() &&
                                        !ui->lineEditStartingPopulationFile->text().isEmpty() && !ui->lineEditXMLMasterFile->text().isEmpty() &&
                                        gaExecutableValid);
    }

    newAct->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    openAct->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    saveAct->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && m_asynchronousGAFileModified && !m_asynchronousGAFileName.isEmpty() && m_asynchronousGAFileNameValid);
    saveAsAct->setEnabled(m_ga == nullptr && m_runMergeXML == 0);
    editSettingsAct->setEnabled(m_ga == nullptr && m_runMergeXML == 0 && !m_asynchronousGAFileName.isEmpty() && m_asynchronousGAFileNameValid);
    exitAct->setEnabled(true);

    if (ui->checkBoxCycleTime->isChecked()) ui->labelCycleTime->setText("Cycle");
    else ui->labelCycleTime->setText("Time");
}

void Wrapper::appendProgress(const std::string &text)
{
    ui->plainTextEditLog->appendPlainText(text);
}

void Wrapper::tryToStopGA()
{
    if (m_ga) m_ga->write("stop\n");
    //#if defined(WIN32) || defined(_WIN32)
    //    if (m_ga) m_ga->closeWriteChannel(); // this seems to be needed on Windows otherwise the write channel is never sent
    //#endif
}

void Wrapper::setProgressValue(int value)
{
    ui->progressBar->setValue(value);
}

void Wrapper::setResultNumber(int number)
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 millisecondsDiff = m_lastResultsTime.msecsTo(now);
    if (millisecondsDiff == 0) return;
    ui->lineEditResultNumber->setText(std::string::number(number));
    int resultsDiff = number - m_lastResultsNumber;
    m_lastResultsTime = now;
    m_lastResultsNumber = number;
    double generationsPerSecond = 1000.0 * double(resultsDiff) / double(millisecondsDiff);
    ui->lineEditGenerationsPerSecond->setText(std::string::number(generationsPerSecond));
}

void Wrapper::setBestScore(double value)
{
    ui->lineEditBestScore->setText(std::string::number(value, 'g', 6));
}

void Wrapper::setEvolveID(uint64_t value)
{
    ui->lineEditRunID->setText(std::string::number(value));
}

void Wrapper::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "AsynchronousGA4");
    m_asynchronousGAFileName = settings.value("asynchronousGAFileFileName", "").toString();
    m_editorFont.fromString(settings.value("editorFont", QFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)).toString()).toString());
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    setCustomTitle();
}

void Wrapper::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "AsynchronousGA4");
    settings.setValue("asynchronousGAFileFileName", m_asynchronousGAFileName);
    settings.setValue("editorFont", m_editorFont.toString());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.sync();
}

void Wrapper::closeEvent (QCloseEvent *event)
{
    if (m_ga)
    {
        QMessageBox::StandardButton ret = QMessageBox::warning(this, "AsynchronousGA4", "GA is running.\nAre you sure you want to close?.", QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No), QMessageBox::No);
        if (ret == QMessageBox::No)
        {
            event->ignore();
            return;
        }
        m_ga->kill();
    }

    if (maybeSave())
    {
        writeSettings();
        event->accept(); // pass the event to the default handler
    }
    else
    {
        event->ignore();
    }
}

void Wrapper::createActions()
{
    newAct = new QAction(QIcon(":/Images/new.svg"), tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &Wrapper::newFile);

    openAct = new QAction(QIcon(":/Images/open.svg"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &Wrapper::open);

    saveAct = new QAction(QIcon(":/Images/save.svg"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &Wrapper::save);

    saveAsAct = new QAction(QIcon(":/Images/save-as.svg"), tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document to disk with new name"));
    connect(saveAsAct, &QAction::triggered, this, &Wrapper::saveAs);

    editSettingsAct = new QAction(QIcon(":/Images/preferences.svg"), tr("Ed&it Settings..."), this);
    editSettingsAct->setShortcuts(QKeySequence::Preferences);
    editSettingsAct->setStatusTip(tr("Open the run specific settings file in an editor"));
    connect(editSettingsAct, &QAction::triggered, this, &Wrapper::editSettings);

    exitAct = new QAction(QIcon(":/Images/exit.svg"), tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &Wrapper::close);

    undoAct = new QAction(QIcon(":/Images/undo.svg"), tr("&Undo"), this);
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setStatusTip(tr("Undo the last operation"));
    connect(undoAct, &QAction::triggered, this, &Wrapper::undo);

    redoAct = new QAction(QIcon(":/Images/redo.svg"), tr("&Redo"), this);
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setStatusTip(tr("Redo the last operation"));
    connect(redoAct, &QAction::triggered, this, &Wrapper::redo);

    cutAct = new QAction(QIcon(":/Images/cut.svg"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, &QAction::triggered, this, &Wrapper::cut);

    copyAct = new QAction(QIcon(":/Images/copy.svg"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, &QAction::triggered, this, &Wrapper::copy);

    pasteAct = new QAction(QIcon(":/Images/paste.svg"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, &QAction::triggered, this, &Wrapper::paste);

    fontAct = new QAction(QIcon(":/Images/font.svg"), tr("&Font"), this);
    fontAct->setStatusTip(tr("Select the font for the text editor windows"));
    connect(fontAct, &QAction::triggered, this, &Wrapper::font);

    aboutAct = new QAction(QIcon(":/Images/icon_design.svg"), tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, &Wrapper::about);
}

void Wrapper::createMenus()
{
    fileMenu = ui->menuBar->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addAction(editSettingsAct);
    //    fileMenu->addAction(printAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = ui->menuBar->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addSeparator();
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addSeparator();

    helpMenu = ui->menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(fontAct);
}

void Wrapper::newFile()
{
    if (maybeSave() == false) return;
    m_asynchronousGAFileName = "AsynchronousGAQ Settings.xml";
    m_asynchronousGAFileModified = false;
    m_asynchronousGAFileNameValid = false;
    ui->lineEditOutputFolder->setText("");
    ui->lineEditParameterFile->setText("");
    ui->lineEditXMLMasterFile->setText("");
    ui->lineEditStartingPopulationFile->setText("");
    ui->lineEditModelConfigurationFile->setText("");
    ui->lineEditModelPopulationFile->setText("");
    ui->lineEditDriverFile->setText("");
    ui->lineEditWorkingFolder->setText("");
    ui->lineEditMergeXMLFile->setText("");
    ui->lineEditGaitSymExecutable->setText("");
    ui->lineEditMergeXMLFile->setText("");
    ui->spinBoxLogLevel->setValue(1);
    ui->spinBoxPort->setValue(8086);
    ui->doubleSpinBoxStartValue->setValue(0);
    ui->doubleSpinBoxStepValue->setValue(1);
    ui->doubleSpinBoxEndValue->setValue(1);
    ui->doubleSpinBoxOutputCycle->setValue(0);
    ui->checkBoxMergeXMLActivate->setChecked(false);
    ui->checkBoxCycleTime->setChecked(false);
    writeSettings(); // makes sure the settings are all up to date
    activateButtons();
}

void Wrapper::openFile(const std::string &fileName)
{
    m_asynchronousGAFileName = fileName;
    m_asynchronousGAFileModified = false;
    m_asynchronousGAFileNameValid = false;
    QFile file(m_asynchronousGAFileName);
    if (file.open(QFile::ReadOnly) == false)
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("open: Unable to open file (read):\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Open File Error"), std::string("open: Unable to open file (read):\n%1").arg(m_asynchronousGAFileName));
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QDomDocument doc;
    if (!doc.setContent(data))
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("open: Unable to parse XML file:\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Open File Error"), std::string("open: Unable to parse XML file:\n%1").arg(m_asynchronousGAFileName));
        return;
    }

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "AsynchronousGA4");
    settings.setValue("asynchronousGAFileFileName", m_asynchronousGAFileName);
    settings.sync();

    QDomElement docElem = doc.documentElement();
    // qWarning("Element name: %s", qPrintable(docElem.tagName()));
    if (docElem.tagName() != "ASYNCHRONOUSGA") return;
    QDomNode n = docElem.firstChild();
    while(!n.isNull())
    {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if (!e.isNull())
        {
            if (e.tagName() == "SETTINGS")
            {
                ui->lineEditOutputFolder->setText(convertToAbsolutePath(e.attribute("outputFolder")));
                ui->lineEditParameterFile->setText(convertToAbsolutePath(e.attribute("parameterFile")));
                ui->lineEditXMLMasterFile->setText(convertToAbsolutePath(e.attribute("xmlMasterFile")));
                ui->lineEditStartingPopulationFile->setText(convertToAbsolutePath(e.attribute("startingPopulationFile")));
                ui->lineEditModelConfigurationFile->setText(convertToAbsolutePath(e.attribute("modelConfigurationFile")));
                ui->lineEditModelPopulationFile->setText(convertToAbsolutePath(e.attribute("modelPopulationFile")));
                ui->lineEditDriverFile->setText(convertToAbsolutePath(e.attribute("driverFile")));
                ui->lineEditWorkingFolder->setText(convertToAbsolutePath(e.attribute("workingFolder")));
                ui->lineEditMergeXMLFile->setText(convertToAbsolutePath(e.attribute("mergeXMLFile")));
                ui->lineEditGaitSymExecutable->setText(convertToAbsolutePath(e.attribute("gaitSymExecutable")));
                ui->lineEditPostMergeScript->setText(convertToAbsolutePath(e.attribute("postMergeScript")));
                ui->lineEditGAExecutable->setText(convertToAbsolutePath(e.attribute("gaExecutable")));
                ui->spinBoxLogLevel->setValue(e.attribute("logLevel").toInt());
                ui->spinBoxPort->setValue(e.attribute("portNumber").toInt());
                ui->doubleSpinBoxStartValue->setValue(e.attribute("startValue").toDouble());
                ui->doubleSpinBoxStepValue->setValue(e.attribute("stepValue").toDouble());
                ui->doubleSpinBoxEndValue->setValue(e.attribute("endValue").toDouble());
                ui->doubleSpinBoxOutputCycle->setValue(e.attribute("outputCycle").toDouble());
                ui->checkBoxMergeXMLActivate->setChecked(QVariant(e.attribute("mergeXMLActivate")).toBool());
                ui->checkBoxCycleTime->setChecked(QVariant(e.attribute("cycle")).toBool());

                m_startExpressionMarker = e.attribute("startExpressionMarker", "{{").toStdString();
                m_endExpressionMarker = e.attribute("endExpressionMarker", "}}").toStdString();
            }
        }
        n = n.nextSibling();
    }
    if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("'%1' read").arg(m_asynchronousGAFileName));
    ui->statusBar->showMessage(std::string("'%1' read").arg(m_asynchronousGAFileName));
    m_asynchronousGAFileModified = false;
    m_asynchronousGAFileNameValid = true;
    activateButtons();
    setCustomTitle();
}

void Wrapper::save()
{
    QFile file(m_asynchronousGAFileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("save: Unable to open file (write):\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Save File Error"), std::string("save: Unable to open file (write):\n%1").arg(m_asynchronousGAFileName));
        return;
    }

    writeSettings(); // makes sure the settings are all up to date

    QDomDocument doc("AsynchronousGA_Settings_Document_0.1");
    QDomProcessingInstruction pi = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(pi);
    QDomElement root = doc.createElement("ASYNCHRONOUSGA");
    doc.appendChild(root);

    QDomElement dataItemsElement = doc.createElement("SETTINGS");
    root.appendChild(dataItemsElement);

    dataItemsElement.setAttribute("outputFolder", convertToRelativePath(ui->lineEditOutputFolder->text()));
    dataItemsElement.setAttribute("parameterFile", convertToRelativePath(ui->lineEditParameterFile->text()));
    dataItemsElement.setAttribute("xmlMasterFile", convertToRelativePath(ui->lineEditXMLMasterFile->text()));
    dataItemsElement.setAttribute("startingPopulationFile", convertToRelativePath(ui->lineEditStartingPopulationFile->text()));
    dataItemsElement.setAttribute("modelConfigurationFile", convertToRelativePath(ui->lineEditModelConfigurationFile->text()));
    dataItemsElement.setAttribute("modelPopulationFile", convertToRelativePath(ui->lineEditModelPopulationFile->text()));
    dataItemsElement.setAttribute("driverFile", convertToRelativePath(ui->lineEditDriverFile->text()));
    dataItemsElement.setAttribute("workingFolder", convertToRelativePath(ui->lineEditWorkingFolder->text()));
    dataItemsElement.setAttribute("mergeXMLFile", convertToRelativePath(ui->lineEditMergeXMLFile->text()));
    dataItemsElement.setAttribute("gaitSymExecutable", convertToRelativePath(ui->lineEditGaitSymExecutable->text()));
    dataItemsElement.setAttribute("postMergeScript", convertToRelativePath(ui->lineEditPostMergeScript->text()));
    dataItemsElement.setAttribute("gaExecutable", convertToRelativePath(ui->lineEditGAExecutable->text()));
    dataItemsElement.setAttribute("logLevel", ui->spinBoxLogLevel->text());
    dataItemsElement.setAttribute("portNumber", ui->spinBoxPort->text());
    dataItemsElement.setAttribute("startValue", ui->doubleSpinBoxStartValue->text());
    dataItemsElement.setAttribute("stepValue", ui->doubleSpinBoxStepValue->text());
    dataItemsElement.setAttribute("endValue", ui->doubleSpinBoxEndValue->text());
    dataItemsElement.setAttribute("outputCycle", ui->doubleSpinBoxOutputCycle->text());
    dataItemsElement.setAttribute("mergeXMLActivate", std::string::number(ui->checkBoxMergeXMLActivate->isChecked() ? 1 : 0));
    dataItemsElement.setAttribute("cycle", std::string::number(ui->checkBoxCycleTime->isChecked() ? 1 : 0));

    dataItemsElement.setAttribute("startExpressionMarker", std::string::fromStdString(m_startExpressionMarker));
    dataItemsElement.setAttribute("endExpressionMarker", std::string::fromStdString(m_endExpressionMarker));

    // and now the actual xml doc
    QByteArray xmlData = doc.toByteArray();
    qint64 bytesWritten = file.write(xmlData);
    if (bytesWritten != xmlData.size())
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("save: Unable to write file (write):\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Save File Error"), std::string("save: Unable to write file (write):\n%1").arg(m_asynchronousGAFileName));
    }
    file.close();
    if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("'%1' written").arg(m_asynchronousGAFileName));
    ui->statusBar->showMessage(std::string("'%1' written").arg(m_asynchronousGAFileName));
    m_asynchronousGAFileModified = false;
    m_asynchronousGAFileNameValid = true;
    activateButtons();
    setCustomTitle();
}

void Wrapper::saveAs()
{
    std::string fileName = QFileDialog::getSaveFileName(this, tr("Save Run Settings File"), m_asynchronousGAFileName, tr("XML Files (*.xml);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        m_asynchronousGAFileName = fileName;
        save();
    }
}

std::string Wrapper::convertToRelativePath(const std::string &filename)
{
    QFileInfo fileInfo(m_asynchronousGAFileName);
    QDir dir = fileInfo.absoluteDir();
    std::string relativePath = QDir::cleanPath(dir.relativeFilePath(filename));
    return relativePath;
}

std::string Wrapper::convertToAbsolutePath(const std::string &filename)
{
    QFileInfo fileInfo(m_asynchronousGAFileName);
    QDir dir = fileInfo.absoluteDir();
    std::string relativePath = QDir::cleanPath(dir.absoluteFilePath(filename));
    return relativePath;
}

std::string Wrapper::existsOnPath(const std::string &filename)
{
    std::string path = qEnvironmentVariable("PATH");
    std::stringList pathFolders = path.split(':');
    for (auto &&pathFolder : pathFolders)
    {
        std::string testFile = QDir(pathFolder).absoluteFilePath(filename);
        QFileInfo testFileInfo(testFile);
        if (testFileInfo.exists() && testFileInfo.isExecutable()) return testFile;
    }
    return std::string();
}

void Wrapper::editSettings()
{
    TextEditDialog textEditDialog(this);
    QFile editFile(m_asynchronousGAFileName);
    if (editFile.open(QFile::ReadOnly) == false)
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("editSettings: Unable to open file (read):\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Open File Error"), std::string("editSettings: Unable to open file (read):\n%1").arg(m_asynchronousGAFileName));
        return;
    }
    QByteArray editFileData = editFile.readAll();
    editFile.close();
    std::string editFileText = std::string::fromUtf8(editFileData);

    textEditDialog.useXMLSyntaxHighlighter();
    textEditDialog.setEditorText(editFileText);

    int status = textEditDialog.exec();

    if (status == QDialog::Accepted) // write the new settings
    {
        if (editFile.open(QFile::WriteOnly) == false)
        {
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("editSettings: Unable to open file (write):\n%1").arg(m_asynchronousGAFileName));
            QMessageBox::warning(this, tr("Open File Error"), std::string("editSettings: Unable to open file (write):\n%1").arg(m_asynchronousGAFileName));
            return;
        }
        editFileData = textEditDialog.editorText().toUtf8();
        editFile.write(editFileData);
        editFile.close();
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("'%1' written").arg(m_asynchronousGAFileName));
        ui->statusBar->showMessage(std::string("'%1' written").arg(m_asynchronousGAFileName));
        openFile(m_asynchronousGAFileName);
    }
    else
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("'%1' not changed").arg(m_asynchronousGAFileName));
        ui->statusBar->showMessage(std::string("'%1' not changed").arg(m_asynchronousGAFileName));
    }
}

void Wrapper::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(cutAct);
    menu.addAction(copyAct);
    menu.addAction(pasteAct);
    menu.exec(event->globalPos());
}

void Wrapper::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_basicTimer.timerId()) { activateButtons(); }
    else { QMainWindow::timerEvent(event); }
}

void Wrapper::undo()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->undo(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->undo(); return; }
}

void Wrapper::redo()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->redo(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->redo(); return; }
}

void Wrapper::cut()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->cut(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->cut(); return; }
}

void Wrapper::copy()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->copy(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->copy(); return; }
}

void Wrapper::paste()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->paste(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->paste(); return; }
}

void Wrapper::font()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, m_editorFont, this);
    if (ok)
    {
        m_editorFont = font;
        setEditorFonts();
        writeSettings();
    }
}

void Wrapper::about()
{
    std::string description(u8"This application coordinates optimisation using \nremote clients to do the hard work.\n\u00a9 William Sellers 2024.\nReleased under GPL v3.");
    std::string buildDate = std::string("Build: %1 %2").arg(__DATE__, __TIME__);
    std::string buildType;
#ifdef NDEBUG
    buildType = "Release";
#else
    buildType = "Debug";
#endif
    std::string buildInformation = QSysInfo::buildAbi();

    QMessageBox msgBox;
    msgBox.setText(tr("<b>AsynchronousGA4</b>: Genetic Algorithm Server"));
    msgBox.setInformativeText(std::string("%1\n%2 %3\n%4").arg(description, buildDate, buildType, buildInformation));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIconPixmap(QIcon(":/Images/icon_design.svg").pixmap(QSize(256, 256))); // going via a QIcon means the scaling happens before the conversion to a pixmap
    int ret = msgBox.exec();
    switch (ret)
    {
    case QMessageBox::Ok:
        // Ok was clicked
        break;
    default:
        // should never be reached
        break;
    }
}

bool Wrapper::maybeSave()
{
    if (m_asynchronousGAFileModified)
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Settings File"), tr("'%1' has been modified.\nDo you want to save your changes?").arg(m_asynchronousGAFileName),
                                   QMessageBox::Save | QMessageBox::Discard| QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) return false;
        if (ret == QMessageBox::Save)
        {
            if (m_asynchronousGAFileNameValid) save();
            else saveAs();
        }
    }
    return true;
}

void Wrapper::menuRequestPath(QPoint pos)
{
    // this should always come from a QLineEdit
    QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender());
    if (lineEdit == nullptr) return;

    QMenu *menu = lineEdit->createStandardContextMenu();
    menu->addSeparator();
    menu->addAction(tr("Edit File..."));


    QPoint gp = lineEdit->mapToGlobal(pos);

    QAction *action = menu->exec(gp);
    while (action)
    {
        if (action->text() == tr("Edit File...") && sender() != ui->lineEditParameterFile)
        {
            TextEditDialog textEditDialog(this);
            std::string fileName = lineEdit->text();
            if (fileName.endsWith(".xml", Qt::CaseInsensitive) || fileName.endsWith(".gaitsym", Qt::CaseInsensitive)) textEditDialog.useXMLSyntaxHighlighter();
            QFile editFile(fileName);
            if (editFile.open(QFile::ReadOnly) == false)
            {
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("menuRequestPath: Unable to open file (read):\n%1").arg(fileName));
                QMessageBox::warning(this, tr("Open File Error"), std::string("menuRequestPath: Unable to open file (read):\n%1").arg(fileName));
                return;
            }
            QByteArray editFileData = editFile.readAll();
            editFile.close();
            std::string editFileText = std::string::fromUtf8(editFileData);

            textEditDialog.setEditorText(editFileText);

            int status = textEditDialog.exec();

            if (status == QDialog::Accepted) // write the new settings
            {
                if (editFile.open(QFile::WriteOnly) == false)
                {
                    if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("menuRequestPath: Unable to open file (write):\n%1").arg(fileName));
                    QMessageBox::warning(this, tr("Open File Error"), std::string("menuRequestPath: Unable to open file (write):\n%1").arg(fileName));
                    return;
                }
                editFileData = textEditDialog.editorText().toUtf8();
                editFile.write(editFileData);
                editFile.close();
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("'%1' written").arg(lineEdit->text()));
                ui->statusBar->showMessage(std::string("'%1' written").arg(lineEdit->text()));
            }
            else
            {
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("'%1' not changed").arg(lineEdit->text()));
                ui->statusBar->showMessage(std::string("'%1' not changed").arg(lineEdit->text()));
            }
            break;
        }
        if (action->text() == tr("Edit File...") && sender() == ui->lineEditParameterFile)
        {

            GAParametersDialog gaParametersDialog(this);
            std::string fileName = lineEdit->text();
            QFile editFile(fileName);
            if (editFile.open(QFile::ReadOnly) == false)
            {
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("menuRequestPath: Unable to open file (read):\n%1").arg(fileName));
                QMessageBox::warning(this, tr("Open File Error"), std::string("menuRequestPath: Unable to open file (read):\n%1").arg(fileName));
                return;
            }
            QByteArray editFileData = editFile.readAll();
            editFile.close();
            std::string editFileText = std::string::fromUtf8(editFileData);

            gaParametersDialog.setEditorText(editFileText);

            int status = gaParametersDialog.exec();

            if (status == QDialog::Accepted) // write the new settings
            {
                if (editFile.open(QFile::WriteOnly) == false)
                {
                    if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("menuRequestPath: Unable to open file (write):\n%1").arg(fileName));
                    QMessageBox::warning(this, tr("Open File Error"), std::string("menuRequestPath: Unable to open file (write):\n%1").arg(fileName));
                    return;
                }
                editFileData = gaParametersDialog.editorText().toUtf8();
                editFile.write(editFileData);
                editFile.close();
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("'%1' written").arg(lineEdit->text()));
                ui->statusBar->showMessage(std::string("'%1' written").arg(lineEdit->text()));
            }
            else
            {
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("'%1' not changed").arg(lineEdit->text()));
                ui->statusBar->showMessage(std::string("'%1' not changed").arg(lineEdit->text()));
            }
            break;
        }
        break;
    }
    delete menu;
}

void Wrapper::setEditorFonts()
{
    QList<QPlainTextEdit *> listQPlainTextEdit = this->findChildren<QPlainTextEdit *>(QRegularExpression(".*", QRegularExpression::CaseInsensitiveOption), Qt::FindChildrenRecursively);
    for (QList<QPlainTextEdit *>::iterator it = listQPlainTextEdit.begin(); it != listQPlainTextEdit.end(); it++) (*it)->setFont(m_editorFont);
}

void Wrapper::setCustomTitle()
{
    QFileInfo fileInfo(m_asynchronousGAFileName);
    std::string windowTitle = fileInfo.absoluteFilePath();
    if (m_asynchronousGAFileModified) windowTitle += "*";
    if (!m_asynchronousGAFileNameValid) windowTitle += "+";
    this->setWindowTitle(windowTitle);
}

void Wrapper::readStandardError()
{
    std::string output = m_ga->readAllStandardError();
    static QRegularExpression re("\\n|\\r");
    std::stringList lines = output.split(re, Qt::SkipEmptyParts);
    for (auto &&it : lines)
    {
        std::stringList tokens = it.split('=');
        if (tokens.size() == 2)
        {
            std::string code = tokens[0].trimmed();
            if (code == "Progress") { setProgressValue(tokens[1].toInt()); continue; }
            if (code == "Return Count") { setResultNumber(tokens[1].toInt()); continue; }
            if (code == "Best Score") { setBestScore(tokens[1].toDouble()); continue; }
            if (code == "Evolve Identifier") { setEvolveID(tokens[1].toULongLong()); continue; }
        }
        appendProgress(it + '\n');
    }
}

void Wrapper::readStandardOutput()
{
    std::string output = m_ga->readAllStandardOutput();
    static QRegularExpression re("\\n|\\r");
    std::stringList lines = output.split(re, Qt::SkipEmptyParts);
    appendProgress(lines.join('\n'));
}

void Wrapper::layoutSpacing(QWidget *container)
{
    QList<QLayout *> list = container->findChildren<QLayout *>(Qt::FindChildrenRecursively);
    int left = 4, top = 4, right = 4, bottom = 4;
    int spacing = 4;
    for (auto &&item : list)
    {
        QMargins margins = item->contentsMargins();
        margins.setLeft(std::min(left, margins.left()));
        margins.setRight(std::min(right, margins.right()));
        margins.setTop(std::min(top, margins.top()));
        margins.setBottom(std::min(bottom, margins.bottom()));
        item->setContentsMargins(margins);
        if (auto w = dynamic_cast<QGridLayout *>(item))
        {
            int hSpacing = w->horizontalSpacing();
            int vSpacing = w->verticalSpacing();
            w->setHorizontalSpacing(std::min(spacing, hSpacing));
            w->setVerticalSpacing(std::min(spacing, vSpacing));
        }
        else
        {
            item->setSpacing(std::min(item->spacing(), spacing));
        }
    }
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

std::string Wrapper::runCommand(const std::string& program, const std::vector<std::string>& args, int *exitStatus)
{
    std::string result;
    std::ostringstream cmd;
    cmd << shellEscape(program);

    for (auto& a : args)
    {
        cmd << " " << shellEscape(a);
    }
    cmd << " 2>&1";  // redirect stderr to stdout

#if defined(_WIN32)
    FILE* pipe = _popen(cmd.str().c_str(), "r");
#else
    FILE* pipe = popen(cmd.str().c_str(), "r");
#endif

    if (!pipe)
    {
        *exitStatus = -1;
        return result;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe)) { result += buffer; }

#if defined(_WIN32)
    *exitStatus = _pclose(pipe);
#else
    *exitStatus = pclose(pipe);
#endif
    return result;
}


std::vector<std::filesystem::path> Wrapper::listFilesMatching(const std::filesystem::path& folder, const std::regex& pattern)
{
    std::vector<std::filesystem::path> results;

    if (!std::filesystem::exists(folder) || !std::filesystem::is_directory(folder)) {
        return results; // return empty if folder doesn't exist
    }

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
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

bool Wrapper::isExecutableFile(const std::filesystem::path& p)
{
    // Must exist and be a regular file
    if (!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p)) {
        return false;
    }

#if defined(_WIN32)
    // Windows: no executable bit, so we check common executable extensions
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".exe" ||
           ext == ".bat" ||
           ext == ".cmd" ||
           ext == ".com" ||
           ext == ".ps1";
#else
    // POSIX: check execute permission for the current user
    return ::access(p.c_str(), X_OK) == 0;
#endif
}

std::string Wrapper::readFile(const std::string &path, std::string *errorMessage)
{
    std::string result;
    // Open the stream to 'lock' the file.
    std::ifstream file(path, std::ios::in | std::ios::binary);
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
        if (errorMessage) { *errorMessage = "I/O error while reading file: " + path; }
        result.errorCode    = FileReadError::ReadError;
        result.errorMessage = "I/O error while reading file: " + path;
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


