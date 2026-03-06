#include "Wrapper.h"

#include "MergeUtil.h"
#include "MergeXML.h"
#include "XMLConverter.h"

#include "pystring.h"

using namespace std::string_literals;

Wrapper::Wrapper()
{
}

void Wrapper::run()
{
    m_currentLoopValue = m_startValue;
    m_currentLoopCount = 0;
    m_lastPopulation = m_modelPopulationFile;
    m_lastConfig = m_modelConfigurationFile;



    while (true)
    {
        runGaitSym(); // run gaitsym and generate ModelState.xml
        runMergeXML();
        runGA();
        runPostMergeScript();
        std::string outputFolder = ui->lineEditOutputFolder->text();
        QDir dir(outputFolder);
        lastConfig = dir.filePath("ModelState.xml");
        if (dir.exists("ModelState.xml") == false)
        {
            m_mergeXMLTimer->stop();
            m_runMergeXML = 0;
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("MergeXML: Unable to find ModelState.xml in:\n%1").arg(outputFolder));
            QMessageBox::warning(this, tr("Directory Read Error"), std::string("MergeXML: Unable to find ModelState.xml in:\n%1").arg(outputFolder));
            activateButtons();
            return;
        }
        std::stringList files = dir.entryList(std::stringList() << "Population*.txt", QDir::Files, QDir::Name);
        if (files.isEmpty())
        {
            m_mergeXMLTimer->stop();
            m_runMergeXML = 0;
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("MergeXML: Unable to find Population*.txt in:\n%1").arg(outputFolder));
            QMessageBox::warning(this, tr("Directory Read Error"), std::string("MergeXML: Unable to find Population*.txt in:\n%1").arg(outputFolder));
            activateButtons();
            return;
        }
        lastPopulation = dir.filePath(files.last());
        m_currentLoopCount++;
        m_currentLoopValue += stepValue;
        if (stepValue >= 0 && (m_currentLoopValue > endValue || fabs(m_currentLoopValue - endValue) < fabs(stepValue) * 0.001)) m_currentLoopValue = endValue;
        if (stepValue < 0 && (m_currentLoopValue < endValue || fabs(m_currentLoopValue - endValue) < fabs(stepValue) * 0.001)) m_currentLoopValue = endValue;
    }

}

void Wrapper::runGA()
{
    std::vector<std::string> arguments{"--parameterFile", m_parameterFile,
                                       "--baseXMLFile", m_xmlMasterFile,
                                       "--startingPopulation", m_startingPopulationFile,
                                       "--outputDirectory", m_outputFolder,
                                       "--serverPort", std::to_string(serverPort),
                                       "--logLevel", std::to_string(m_logLevel)};
    int status = runCommand(gaExecutable, arguments);
    if (status)
    {
        std::cerr << "Error " << status << " running: " << gaExecutable << pystring::join(" "s, arguments) << "\n";
        std::exit(status);
    }
}

void Wrapper::runMergeXML()
{
    if ((m_stepValue >= 0 && m_currentLoopValue >= m_endValue) || (m_stepValue < 0 && m_currentLoopValue <= m_endValue))
    {
        std::cerr << "Finished loop\n";
        std::exit(0);
    }

    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Running MergeXML");
    ui->statusBar->showMessage("Running MergeXML");
    std::string lastPopulation;
    std::string lastConfig;
    if (m_runMergeXML == 1)
    {
        // get the population and config from the models
        m_runMergeXML = 2;
        QFileInfo modelConfigurationFile(ui->lineEditModelConfigurationFile->text());
        QFileInfo modelPopulationFile(ui->lineEditModelPopulationFile->text());
        lastPopulation = modelPopulationFile.absoluteFilePath();
        lastConfig = modelConfigurationFile.absoluteFilePath();
        m_currentLoopValue = startValue;
        m_currentLoopCount = 0;
    }
    else
    {
        // get the population and config from the output of the previous runGA
        runGaitSym(); // run gaitsym and generate ModelState.xml
        std::string outputFolder = ui->lineEditOutputFolder->text();
        QDir dir(outputFolder);
        lastConfig = dir.filePath("ModelState.xml");
        if (dir.exists("ModelState.xml") == false)
        {
            m_mergeXMLTimer->stop();
            m_runMergeXML = 0;
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("MergeXML: Unable to find ModelState.xml in:\n%1").arg(outputFolder));
            QMessageBox::warning(this, tr("Directory Read Error"), std::string("MergeXML: Unable to find ModelState.xml in:\n%1").arg(outputFolder));
            activateButtons();
            return;
        }
        std::stringList files = dir.entryList(std::stringList() << "Population*.txt", QDir::Files, QDir::Name);
        if (files.isEmpty())
        {
            m_mergeXMLTimer->stop();
            m_runMergeXML = 0;
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("MergeXML: Unable to find Population*.txt in:\n%1").arg(outputFolder));
            QMessageBox::warning(this, tr("Directory Read Error"), std::string("MergeXML: Unable to find Population*.txt in:\n%1").arg(outputFolder));
            activateButtons();
            return;
        }
        lastPopulation = dir.filePath(files.last());
        m_currentLoopCount++;
        m_currentLoopValue += stepValue;
        if (stepValue >= 0 && (m_currentLoopValue > endValue || fabs(m_currentLoopValue - endValue) < fabs(stepValue) * 0.001)) m_currentLoopValue = endValue;
        if (stepValue < 0 && (m_currentLoopValue < endValue || fabs(m_currentLoopValue - endValue) < fabs(stepValue) * 0.001)) m_currentLoopValue = endValue;
    }
    ui->lineEditCurrentLoopValue->setText(std::string::number(m_currentLoopValue, 'f', 3));
    ui->lineEditCurrentLoopCount->setText(std::string::number(m_currentLoopCount, 'f', 3));
    QFileInfo driverFile(ui->lineEditDriverFile->text());
    QFileInfo workingFolder(ui->lineEditWorkingFolder->text());
    QFile mergeXMLFile(ui->lineEditMergeXMLFile->text());
    if (mergeXMLFile.open(QFile::ReadOnly) == false)
    {
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("MergeXML: Unable to open file:\n%1").arg(ui->lineEditMergeXMLFile->text()));
        QMessageBox::warning(this, tr("Open File Error"), std::string("MergeXML: Unable to open file:\n%1").arg(ui->lineEditMergeXMLFile->text()));
        activateButtons();
        return;
    }
    QByteArray mergeXMLFileData = mergeXMLFile.readAll();
    mergeXMLFile.close();
    std::string mergeXMLCommands = std::string::fromUtf8(mergeXMLFileData);
    QDateTime time = QDateTime::currentDateTime();
    std::string subFolder(std::string("%1_Run_%2").arg(m_currentLoopCount, 4, 10, QChar('0')).arg(time.toString("yyyy-MM-dd_HH.mm.ss")));
    std::string outputFolder = QDir(workingFolder.absoluteFilePath()).filePath(subFolder);
    ui->lineEditOutputFolder->setText(outputFolder);
    QDir().mkpath(outputFolder);
    if (QFileInfo(outputFolder).isDir() == false)
    {
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("MergeXML: Unable to create folder:\n%1").arg(outputFolder));
        QMessageBox::warning(this, tr("Create Folder Error"), std::string("MergeXML: Unable to create folder:\n%1").arg(outputFolder));
        activateButtons();
        return;
    }
    std::string workingConfig = QDir(outputFolder).filePath("workingConfig.xml");
    ui->lineEditXMLMasterFile->setText(workingConfig);
    QFileInfo firstConfigFile(ui->lineEditModelConfigurationFile->text());
    std::string replace1 = mergeXMLCommands.replace("MODEL_CONFIG_FILE", lastConfig);
    std::string replace2 = replace1.replace("DRIVER_CONFIG_FILE", driverFile.absoluteFilePath());
    std::string replace3 = replace2.replace("WORKING_CONFIG_FILE", workingConfig);
    std::string replace4 = replace3.replace("CURRENT_LOOP_VALUE", std::string::number(m_currentLoopValue, 'g', 17));
    std::string replace5 = replace4.replace("FIRST_CONFIG_FILE", firstConfigFile.absoluteFilePath());
    MergeXML mergeXML;
    mergeXML.ExecuteInstructionFile(replace5.toUtf8().constData());
    if (mergeXML.errorMessageList().size())
    {
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0)
        {
            appendProgress(std::string("MergeXML: Unable to parse file:\n%1").arg(ui->lineEditMergeXMLFile->text()));
            for (auto &&it : mergeXML.errorMessageList())
            {
                appendProgress(std::string("Line %1: %2\n").arg(it.first).arg(std::string::fromStdString(it.second)));
                if (ui->spinBoxLogLevel->value() > 1)
                {
                    auto tokensIt = mergeXML.errorList().find(it.first);
                    if (tokensIt != mergeXML.errorList().end())
                    {
                        std::string tokens = pystring::join(" "s, tokensIt->second);
                        appendProgress(std::string("%1").arg(std::string::fromStdString(tokens)));
                    }
                }
            }
        }
        QMessageBox::warning(this, tr("Create Folder Error"), std::string("MergeXML: Unable to parse file:\n%1").arg(ui->lineEditMergeXMLFile->text()));
        activateButtons();
        return;
    }
    std::string newMergeXML = QDir(outputFolder).filePath("workingMergeXML.txt");
    QFile file(newMergeXML);
    if (!file.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, std::string("Open File Error %1").arg(newMergeXML), file.errorString());
        activateButtons();
        return;
    }
    file.write(replace5.toUtf8());
    file.close();
    std::string mergeXMLStatusFile = QDir(outputFolder).filePath("mergeXMLStatus.txt");
    QFile file2(mergeXMLStatusFile);
    if (!file2.open(QFile::WriteOnly))
    {
        QMessageBox::warning(this, std::string("Open File Error %1").arg(newMergeXML), file.errorString());
        activateButtons();
        return;
    }
    QTextStream stream(&file2);
    double outputCycle = ui->doubleSpinBoxOutputCycle->value();
    bool cycle = ui->checkBoxCycleTime->isChecked();
    stream << "startValue " << startValue << "\n";
    stream << "stepValue " << stepValue << "\n";
    stream << "endValue " << endValue << "\n";
    stream << "currentLoopCount " << m_currentLoopCount << "\n";
    if (cycle) stream << "outputCycle " << outputCycle << "\n";
    if (cycle) stream << "outputTime " << outputCycle << "\n";
    stream << "CURRENT_LOOP_VALUE " << m_currentLoopValue << "\n";
    stream << "MODEL_CONFIG_FILE " << lastConfig << "\n";
    stream << "DRIVER_CONFIG_FILE " << driverFile.absoluteFilePath() << "\n";
    stream << "WORKING_CONFIG_FILE " << workingConfig << "\n";
    stream << "FIRST_CONFIG_FILE " << firstConfigFile.absoluteFilePath() << "\n";
    file2.close();
    ui->lineEditXMLMasterFile->setText(workingConfig);
    std::string newPopulation = QDir(outputFolder).filePath("workingPopulation.txt");
    QFile::copy(lastPopulation, newPopulation);
    ui->lineEditStartingPopulationFile->setText(newPopulation);
    runPostMergeScript();
    runGA();
}

void Wrapper::runPostMergeScript()
{
    std::string mergeScriptExecutable = ui->lineEditPostMergeScript->text();
    if (mergeScriptExecutable.isEmpty()) return;

    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Running Post Merge Script");
    ui->statusBar->showMessage("Running Post Merge Script");

    QFileInfo mergeScriptExecutableInfo(mergeScriptExecutable);
    bool mergeScriptExecutableValid = false;
    if (mergeScriptExecutableInfo.exists() && mergeScriptExecutableInfo.isFile()) mergeScriptExecutableValid = true;
    if (!mergeScriptExecutableValid)
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("runPostMergeScript: Unable to find \"%1\"").arg(mergeScriptExecutable));
        QMessageBox::warning(this, tr("Run Post Merge Script Error"), std::string("runPostMergeScript: Unable to find \"%1\"").arg(mergeScriptExecutable));
        return;
    }
    std::stringList arguments;
    QProcess mergeScript;
    std::string interpreter;
    if (mergeScriptExecutableInfo.isExecutable())
    {
        arguments << "--startingPopulationFile" << ui->lineEditStartingPopulationFile->text()
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
    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Running GaitSym");
    ui->statusBar->showMessage("Running GaitSym");
    double outputCycle = ui->doubleSpinBoxOutputCycle->value();
    bool cycleFlag = ui->checkBoxCycleTime->isChecked();
    std::string outputFolder = ui->lineEditOutputFolder->text();
    QDir dir(outputFolder);
    std::stringList files = dir.entryList(std::stringList() << "BestGenome*.txt", QDir::Files, QDir::Name);
    if (files.size() < 1)
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("runGaitSym: Unable to find BestGenome*.txt"));
        QMessageBox::warning(this, tr("Run GaitSym Error"), std::string("runGaitSym: Unable to find BestGenome*.txt"));
        return;
    }
    std::string inputGenome = dir.filePath(files.last());
    std::string inputXML = dir.filePath("workingConfig.xml");
    QFileInfo inputGenomeFile(inputGenome);
    std::string outputXML = dir.filePath(inputGenomeFile.completeBaseName() + std::string(".xml"));
    std::vector<double> genes, lowBounds, highBounds, gaussianSDs;
    std::vector<int> circularMutationFlags;
    double fitness;
    int genomeType;
    MergeUtil::readGenome(inputGenome.toStdString(), &genes, &lowBounds, &highBounds, &gaussianSDs, &circularMutationFlags, &fitness, &genomeType);
    XMLConverter xmlConverter;
    xmlConverter.LoadBaseXMLFile(inputXML.toStdString());
    xmlConverter.ApplyGenome(genes);
    std::string formattedXML;
    xmlConverter.GetFormattedXML(&formattedXML);
    std::ofstream outputXMLFile(outputXML.toStdString(), std::ios::binary);
    outputXMLFile.write(formattedXML.data(), formattedXML.size());
    outputXMLFile.close();

    std::string modelStateFileName = dir.filePath("ModelState.xml");
    std::string gaitSymExecutable = ui->lineEditGaitSymExecutable->text();
    QFileInfo gaitSymExecutableInfo(gaitSymExecutable);
    bool gaitSymExecutableValid = false;
    if (gaitSymExecutableInfo.exists() && gaitSymExecutableInfo.isFile() && gaitSymExecutableInfo.isExecutable()) gaitSymExecutableValid = true;
    if (!gaitSymExecutableValid)
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("runGaitSym: Unable to find \"%1\"").arg(gaitSymExecutable));
        QMessageBox::warning(this, tr("Run GaitSym Error"), std::string("runGaitSym: Unable to find \"%1\"").arg(gaitSymExecutable));
        return;
    }
    std::stringList arguments;
    std::string outputCycleArgument;
    if (cycleFlag) outputCycleArgument = "--outputModelStateAtCycle";
    else outputCycleArgument = "--outputModelStateAtTime";
    arguments << "--config" << outputXML
              << outputCycleArgument << std::string::number(outputCycle, 'g', 17)
              << "--modelState" << modelStateFileName;
    QProcess gaitsym;
    gaitsym.start(gaitSymExecutable, arguments);
    if (!gaitsym.waitForStarted())
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("runGaitSym: Unable to start \"%1\"").arg(gaitSymExecutable));
        QMessageBox::warning(this, tr("Run GaitSym Error"), std::string("runGaitSym: Unable to start \"%1\"").arg(gaitSymExecutable));
        return;
    }
    int msecs = 60 * 60 * 1000;
    if (!gaitsym.waitForFinished(msecs)) // this should work although if gaitsym finished really quickly it is not guaranteed
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("runGaitSym: Timeout \"%1\"").arg(gaitSymExecutable));
        QMessageBox::warning(this, tr("Run GaitSym Error"), std::string("runGaitSym: Timeout \"%1\"").arg(gaitSymExecutable));
        return;
    }
    QByteArray stdOut = gaitsym.readAllStandardOutput();
    if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string::fromStdString(stdOut.toStdString()));
    QByteArray stdError = gaitsym.readAllStandardError();
    if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string::fromStdString(stdError.toStdString()));
    QFileInfo modelStateInfo(modelStateFileName);
    if (!modelStateInfo.exists() && !modelStateInfo.isFile())
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(std::string("runGaitSym: Unable to create:\n%1").arg(modelStateFileName));
        QMessageBox::warning(this, tr("Run GaitSym Error"), std::string("runGaitSym: Unable to create:\n%1").arg(modelStateFileName));
        return;
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

int Wrapper::runCommand(const std::string& program, const std::vector<std::string>& args)
{
    std::ostringstream cmd;
    cmd << shellEscape(program);

    for (auto& a : args)
    {
        cmd << " " << shell_escape(a);
    }

    return std::system(cmd.str().c_str());
}

