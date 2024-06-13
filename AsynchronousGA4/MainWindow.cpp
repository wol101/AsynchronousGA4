#include "MainWindow.h"
#include "./ui_MainWindow.h"

#include "Utilities.h"
#include "TextEditDialog.h"
#include "GAParametersDialog.h"

#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QStringList>
#include <QByteArray>
#include <QSettings>
#include <qglobal.h>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QFontDialog>
#include <QRegularExpression>
#include <QDomDocument>

using namespace std::string_literals;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_redirect = std::make_unique<cerrRedirect>(m_capturedCerr.rdbuf());

    m_editorFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_mergeXMLTimer = new QTimer(this); // this timer
    connect(m_mergeXMLTimer, SIGNAL(timeout()), this, SLOT(runMergeXML()));

    // adjust the appearance of the main window
    ui->mainToolBar->hide();

    // set up the menus
    createActions();
    createMenus();

    // set up the file browse slots
    connect(ui->pushButtonParameterFile, SIGNAL(clicked()), this, SLOT(pushButtonParameterFileClicked()));
    connect(ui->pushButtonStartingPopulationFile, SIGNAL(clicked()), this, SLOT(pushButtonStartingPopulationFileClicked()));
    connect(ui->pushButtonXMLMasterFile, SIGNAL(clicked()), this, SLOT(pushButtonXMLMasterFileClicked()));
    connect(ui->pushButtonOutputFolder, SIGNAL(clicked()), this, SLOT(pushButtonOutputFolderClicked()));
    connect(ui->pushButtonModelConfigurationFile, SIGNAL(clicked()), this, SLOT(pushButtonModelConfigurationFileClicked()));
    connect(ui->pushButtonModelPopulationFile, SIGNAL(clicked()), this, SLOT(pushButtonModelPopulationFileClicked()));
    connect(ui->pushButtonDriverFile, SIGNAL(clicked()), this, SLOT(pushButtonDriverFileClicked()));
    connect(ui->pushButtonWorkingFolder, SIGNAL(clicked()), this, SLOT(pushButtonWorkingFolderClicked()));
    connect(ui->pushButtonMergeXMLFile, SIGNAL(clicked()), this, SLOT(pushButtonMergeXMLFileClicked()));
    connect(ui->pushButtonGaitSymExecutable, SIGNAL(clicked()), this, SLOT(pushButtonGaitSymExecutableClicked()));
    connect(ui->pushButtonMergeScriptExecutable, SIGNAL(clicked()), this, SLOT(pushButtonMergeScriptExecutableClicked()));
    connect(ui->pushButtonGAExecutable, SIGNAL(clicked()), this, SLOT(pushButtonGAExecutableClicked()));
    connect(ui->pushButtonStart, SIGNAL(clicked()), this, SLOT(pushButtonStartClicked()));
    connect(ui->pushButtonStop, SIGNAL(clicked()), this, SLOT(pushButtonStopClicked()));

    // handle the check box
    connect(ui->checkBoxMergeXMLActivate, SIGNAL(clicked()), this, SLOT(checkBoxMergeXMLActivateClicked()));

    // and this updates the buttons too
    connect(ui->lineEditOutputFolder, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditOutputFolderTextChanged(const QString &)));
    connect(ui->lineEditParameterFile, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditParameterFileTextChanged(const QString &)));
    connect(ui->lineEditXMLMasterFile, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditXMLMasterFileTextChanged(const QString &)));
    connect(ui->lineEditStartingPopulationFile, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditStartingPopulationFileTextChanged(const QString &)));
    connect(ui->lineEditModelConfigurationFile, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditModelConfigurationFileTextChanged(const QString &)));
    connect(ui->lineEditModelPopulationFile, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditModelPopulationFileTextChanged(const QString &)));
    connect(ui->lineEditDriverFile, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditDriverFileTextChanged(const QString &)));
    connect(ui->lineEditMergeXMLFile, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditMergeXMLFileTextChanged(const QString &)));
    connect(ui->lineEditGaitSymExecutable, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditGaitSymExecutableTextChanged(const QString &)));
    connect(ui->lineEditMergeScriptExecutable, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditMergeScriptExecutableTextChanged(const QString &)));
    connect(ui->lineEditGAExecutable, SIGNAL(textChanged(const QString &)), this, SLOT(lineEditGAExecutableTextChanged(const QString &)));
    connect(ui->spinBoxLogLevel, SIGNAL(textChanged(const QString &)), this, SLOT(spinBoxLogLevelTextChanged(const QString &)));
    connect(ui->spinBoxPort, SIGNAL(textChanged(const QString &)), this, SLOT(spinBoxTextChanged(const QString &)));
    connect(ui->doubleSpinBoxEndValue, SIGNAL(textChanged(const QString &)), this, SLOT(spinBoxTextChanged(const QString &)));
    connect(ui->doubleSpinBoxOutputCycle, SIGNAL(textChanged(const QString &)), this, SLOT(spinBoxTextChanged(const QString &)));
    connect(ui->doubleSpinBoxStartValue, SIGNAL(textChanged(const QString &)), this, SLOT(spinBoxTextChanged(const QString &)));
    connect(ui->doubleSpinBoxStepValue, SIGNAL(textChanged(const QString &)), this, SLOT(spinBoxTextChanged(const QString &)));
    connect(ui->checkBoxMergeXMLActivate, SIGNAL(stateChanged(int)), this, SLOT(checkBoxStateChanged(int)));
    connect(ui->checkBoxCycleTime, SIGNAL(stateChanged(int)), this, SLOT(checkBoxStateChanged(int)));

    // put a new context menu on the linedit widgets that point to file names
    QList<QLineEdit *> listQLineEdit = this->findChildren<QLineEdit *>(QString(), Qt::FindChildrenRecursively);
    for (QList<QLineEdit *>::iterator it = listQLineEdit.begin(); it != listQLineEdit.end(); it++)
    {
        if ((*it) == ui->lineEditWorkingFolder || (*it) == ui->lineEditOutputFolder ||
            (*it) == ui->lineEditGaitSymExecutable || (*it) == ui->lineEditGAExecutable ||
            (*it) == ui->lineEditMergeScriptExecutable) continue;
        (*it)->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect((*it), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(menuRequestPath(QPoint)));
    }

    readSettings();
    setEditorFonts();

    QString asynchronousGAFileName = m_asynchronousGAFileName;
    newFile();
    QFileInfo fileInfo(asynchronousGAFileName);
    if (fileInfo.exists() && fileInfo.isFile()) open(asynchronousGAFileName);

    // and sort out the buttons
    activateButtons();

    ui->statusBar->showMessage("Ready");
    m_asynchronousGAFileModified = false;
    setCustomTitle();

    // and this timer just makes sure that buttons are regularly updated
    m_basicTimer.start(1000, Qt::CoarseTimer, this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::pushButtonParameterFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Parameter File"), ui->lineEditParameterFile->text(), tr("Text Files (*.txt *.tab);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        ui->lineEditParameterFile->setText(fileName);
    }
}

void MainWindow::pushButtonXMLMasterFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select XML Master File"), ui->lineEditXMLMasterFile->text(), tr("GaitSym XML Files (*.gaitsym *.xml);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        ui->lineEditXMLMasterFile->setText(fileName);
    }
}

void MainWindow::pushButtonStartingPopulationFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Starting Population File"), ui->lineEditStartingPopulationFile->text(), tr("Text Files (*.txt *.tab);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        ui->lineEditStartingPopulationFile->setText(fileName);
    }
}

void MainWindow::pushButtonOutputFolderClicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Select GA Output Folder"), ui->lineEditOutputFolder->text());
    if (!fileName.isEmpty())
    {
        ui->lineEditOutputFolder->setText(fileName);
    }
}

void MainWindow::pushButtonModelConfigurationFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Model Configuration File"), ui->lineEditModelConfigurationFile->text(), tr("GaitSym XML Files (*.gaitsym *.xml);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        ui->lineEditModelConfigurationFile->setText(fileName);
    }
}

void MainWindow::pushButtonModelPopulationFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Model Population File"), ui->lineEditModelPopulationFile->text(), tr("Text Files (*.txt *.tab);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        ui->lineEditModelPopulationFile->setText(fileName);
    }
}

void MainWindow::pushButtonDriverFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the Driver File"), ui->lineEditDriverFile->text(), tr("GaitSym XML Files (*.gaitsym *.xml);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        ui->lineEditDriverFile->setText(fileName);
    }
}

void MainWindow::pushButtonWorkingFolderClicked()
{
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Select MergeXML Working Folder"), ui->lineEditWorkingFolder->text());
    if (!fileName.isEmpty())
    {
        QString lastText = ui->lineEditWorkingFolder->text();
        if (lastText != fileName)
        {
            if (ui->lineEditParameterFile->text().startsWith(lastText)) ui->lineEditParameterFile->setText(ui->lineEditParameterFile->text().replace(lastText, fileName));
            if (ui->lineEditStartingPopulationFile->text().startsWith(lastText)) ui->lineEditStartingPopulationFile->setText(ui->lineEditStartingPopulationFile->text().replace(lastText, fileName));
            if (ui->lineEditXMLMasterFile->text().startsWith(lastText)) ui->lineEditXMLMasterFile->setText(ui->lineEditXMLMasterFile->text().replace(lastText, fileName));
            if (ui->lineEditOutputFolder->text().startsWith(lastText)) ui->lineEditOutputFolder->setText(ui->lineEditOutputFolder->text().replace(lastText, fileName));
            if (ui->lineEditModelConfigurationFile->text().startsWith(lastText)) ui->lineEditModelConfigurationFile->setText(ui->lineEditModelConfigurationFile->text().replace(lastText, fileName));
            if (ui->lineEditModelPopulationFile->text().startsWith(lastText)) ui->lineEditModelPopulationFile->setText(ui->lineEditModelPopulationFile->text().replace(lastText, fileName));
            if (ui->lineEditDriverFile->text().startsWith(lastText)) ui->lineEditDriverFile->setText(ui->lineEditDriverFile->text().replace(lastText, fileName));
            if (ui->lineEditMergeXMLFile->text().startsWith(lastText)) ui->lineEditMergeXMLFile->setText(ui->lineEditMergeXMLFile->text().replace(lastText, fileName));
            if (m_asynchronousGAFileName.startsWith(lastText)) m_asynchronousGAFileName = m_asynchronousGAFileName.replace(lastText, fileName);
            m_asynchronousGAFileNameValid = false;
            m_asynchronousGAFileModified = true;
            ui->lineEditWorkingFolder->setText(fileName);
            setCustomTitle();
        }
    }
}

void MainWindow::pushButtonMergeXMLFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the MergeXML File"), ui->lineEditMergeXMLFile->text(), tr("Text Files (*.txt *.tab);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        ui->lineEditMergeXMLFile->setText(fileName);
    }
}

void MainWindow::pushButtonGAExecutableClicked()
{
#if defined(_WIN32) || defined(WIN32)
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the GA executable"), ui->lineEditGAExecutable->text(), tr("Executable Files (*.exe);;Any File (*.* *)"));
#else
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the GA executable"), ui->lineEditGAExecutable->text(), tr("Executable Files (*)"));
#endif
    if (!fileName.isEmpty())
    {
        ui->lineEditGAExecutable->setText(fileName);
    }
}

void MainWindow::pushButtonGaitSymExecutableClicked()
{
#if defined(_WIN32) || defined(WIN32)
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the GaitSym executable"), ui->lineEditGaitSymExecutable->text(), tr("Executable Files (*.exe);;Any File (*.* *)"));
#else
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the GaitSym executable"), ui->lineEditGaitSymExecutable->text(), tr("Executable Files (*)"));
#endif
    if (!fileName.isEmpty())
    {
        ui->lineEditGaitSymExecutable->setText(fileName);
    }
}

void MainWindow::pushButtonMergeScriptExecutableClicked()
{
#if defined(_WIN32) || defined(WIN32)
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the Merge Script executable"), ui->lineEditMergeScriptExecutable->text(), tr("Executable Files (*.exe);;Any File (*.* *)"));
#else
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select the Merge Script executable"), ui->lineEditMergeScriptExecutable->text(), tr("Executable Files (*)"));
#endif
    if (!fileName.isEmpty())
    {
        ui->lineEditMergeScriptExecutable->setText(fileName);
    }
}


void MainWindow::checkBoxMergeXMLActivateClicked()
{
    activateButtons();
}

void MainWindow::pushButtonStartClicked()
{
    Q_ASSERT(m_ga == nullptr);
    Q_ASSERT(!ui->lineEditParameterFile->text().isEmpty());
    writeSettings();

    if (ui->checkBoxMergeXMLActivate->isChecked())
    {
        m_runMergeXML = 1;
        m_currentLoopValue = ui->doubleSpinBoxStartValue->value();
        m_mergeXMLTimer->start(1000);
    }
    else
    {
        runGA();
    }
    activateButtons();
}

void MainWindow::runGA()
{
    Q_ASSERT(m_ga == nullptr);

    bool gaExecutableValid = false;
    QFileInfo gaExecutableInfo(ui->lineEditGAExecutable->text());
    if (gaExecutableInfo.exists() && gaExecutableInfo.isFile() && gaExecutableInfo.isExecutable()) gaExecutableValid = true;
    if (!gaExecutableValid)
    {
        appendProgress("Error running the GA: GA execujtable not valid");
        ui->statusBar->showMessage("Error running the GA");
        return;
    }

    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Running the GA");
    ui->statusBar->showMessage("Running the GA");
    ui->lineEditBestScore->setText("");
    ui->lineEditResultNumber->setText("");
    ui->lineEditGenerationsPerSecond->setText("");
    m_lastResultsTime = QDateTime::currentDateTime();
    m_lastResultsNumber = -1;
    // just run the GA
    QFileInfo parameterFile(ui->lineEditParameterFile->text());
    QFileInfo startingPopulationFile(ui->lineEditStartingPopulationFile->text());
    QFileInfo xmlMasterFile(ui->lineEditXMLMasterFile->text());
    QFileInfo outputFolder(ui->lineEditOutputFolder->text());
    int logLevel = ui->spinBoxLogLevel->value();
    int serverPort = ui->spinBoxPort->value();

    QString parentDir = parameterFile.canonicalPath();
    if (m_runMergeXML) parentDir = ui->lineEditWorkingFolder->text();
    QDir::setCurrent(parentDir);

    m_xmlMasterFile = xmlMasterFile.absoluteFilePath().toStdString();
    m_outputFolder = outputFolder.absoluteFilePath().toStdString();
    m_parameterFile = parameterFile.absoluteFilePath().toStdString();
    m_startingPopulationFile = startingPopulationFile.absoluteFilePath().toStdString();

    m_ga = new QProcess(this);
    QString program = gaExecutableInfo.absoluteFilePath();
    QStringList arguments;
    arguments << "--parameterFile" << m_parameterFile.c_str()
              << "--baseXMLFile" << m_xmlMasterFile.c_str()
              << "--startingPopulation" << m_startingPopulationFile.c_str()
              << "--outputDirectory" << m_outputFolder.c_str()
              << "--serverPort" << QString("%1").arg(serverPort)
              << "--logLevel" << QString("%1").arg(logLevel);

    connect(m_ga, &QProcess::readyReadStandardOutput, this, &MainWindow::readStandardOutput);
    connect(m_ga, &QProcess::readyReadStandardError, this, &MainWindow::readStandardError);
    connect(m_ga, &QProcess::finished, this, &MainWindow::handleFinished);

    m_ga->start(program, arguments, QIODeviceBase::ReadWrite | QIODeviceBase::Unbuffered);
    if (m_ga->waitForStarted(10000) == false)
    {
        m_ga->kill();
        delete m_ga;
        m_ga = nullptr;
        appendProgress("Error starting up the GA");
    }
}

void MainWindow::runMergeXML()
{
    if (m_ga) return;
    if (m_capturedCerr.str().size()) { appendProgress(QString::fromStdString(m_capturedCerr.str())); m_capturedCerr.clear(); }

    double startValue = ui->doubleSpinBoxStartValue->value();
    double stepValue = ui->doubleSpinBoxStepValue->value();
    double endValue = ui->doubleSpinBoxEndValue->value();
    if ((stepValue >= 0 && m_currentLoopValue >= endValue) || (stepValue < 0 && m_currentLoopValue <= endValue))
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress("MergeXML finished");
        ui->statusBar->showMessage("MergeXML finished");
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        activateButtons();
        return;
    }

    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Running MergeXML");
    ui->statusBar->showMessage("Running MergeXML");
    QString lastPopulation;
    QString lastConfig;
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
        QString outputFolder = ui->lineEditOutputFolder->text();
        QDir dir(outputFolder);
        lastConfig = dir.filePath("ModelState.xml");
        if (dir.exists("ModelState.xml") == false)
        {
            m_mergeXMLTimer->stop();
            m_runMergeXML = 0;
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("MergeXML: Unable to find ModelState.xml in:\n%1").arg(outputFolder));
            QMessageBox::warning(this, tr("Directory Read Error"), QString("MergeXML: Unable to find ModelState.xml in:\n%1").arg(outputFolder));
            activateButtons();
            return;
        }
        QStringList files = dir.entryList(QStringList() << "Population*.txt", QDir::Files, QDir::Name);
        if (files.isEmpty())
        {
            m_mergeXMLTimer->stop();
            m_runMergeXML = 0;
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("MergeXML: Unable to find Population*.txt in:\n%1").arg(outputFolder));
            QMessageBox::warning(this, tr("Directory Read Error"), QString("MergeXML: Unable to find Population*.txt in:\n%1").arg(outputFolder));
            activateButtons();
            return;
        }
        lastPopulation = dir.filePath(files.last());
        m_currentLoopCount++;
        m_currentLoopValue += stepValue;
        if (stepValue >= 0 && (m_currentLoopValue > endValue || fabs(m_currentLoopValue - endValue) < fabs(stepValue) * 0.001)) m_currentLoopValue = endValue;
        if (stepValue < 0 && (m_currentLoopValue < endValue || fabs(m_currentLoopValue - endValue) < fabs(stepValue) * 0.001)) m_currentLoopValue = endValue;
    }
    ui->lineEditCurrentLoopValue->setText(QString::number(m_currentLoopValue, 'f', 3));
    ui->lineEditCurrentLoopCount->setText(QString::number(m_currentLoopCount, 'f', 3));
    QFileInfo driverFile(ui->lineEditDriverFile->text());
    QFileInfo workingFolder(ui->lineEditWorkingFolder->text());
    QFile mergeXMLFile(ui->lineEditMergeXMLFile->text());
    if (mergeXMLFile.open(QFile::ReadOnly) == false)
    {
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("MergeXML: Unable to open file:\n%1").arg(ui->lineEditMergeXMLFile->text()));
        QMessageBox::warning(this, tr("Open File Error"), QString("MergeXML: Unable to open file:\n%1").arg(ui->lineEditMergeXMLFile->text()));
        activateButtons();
        return;
    }
    QByteArray mergeXMLFileData = mergeXMLFile.readAll();
    mergeXMLFile.close();
    QString mergeXMLCommands = QString::fromUtf8(mergeXMLFileData);
    QDateTime time = QDateTime::currentDateTime();
    QString subFolder(QString("%1_Run_%2").arg(m_currentLoopCount, 4, 10, QChar('0')).arg(time.toString("yyyy-MM-dd_HH.mm.ss")));
    QString outputFolder = QDir(workingFolder.absoluteFilePath()).filePath(subFolder);
    ui->lineEditOutputFolder->setText(outputFolder);
    QDir().mkpath(outputFolder);
    if (QFileInfo(outputFolder).isDir() == false)
    {
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("MergeXML: Unable to create folder:\n%1").arg(outputFolder));
        QMessageBox::warning(this, tr("Create Folder Error"), QString("MergeXML: Unable to create folder:\n%1").arg(outputFolder));
        activateButtons();
        return;
    }
    QString workingConfig = QDir(outputFolder).filePath("workingConfig.xml");
    ui->lineEditXMLMasterFile->setText(workingConfig);
    QFileInfo firstConfigFile(ui->lineEditModelConfigurationFile->text());
    QString replace1 = mergeXMLCommands.replace("MODEL_CONFIG_FILE", lastConfig);
    QString replace2 = replace1.replace("DRIVER_CONFIG_FILE", driverFile.absoluteFilePath());
    QString replace3 = replace2.replace("WORKING_CONFIG_FILE", workingConfig);
    QString replace4 = replace3.replace("CURRENT_LOOP_VALUE", QString::number(m_currentLoopValue, 'g', 17));
    QString replace5 = replace4.replace("FIRST_CONFIG_FILE", firstConfigFile.absoluteFilePath());

    QString newMergeXML = QDir(outputFolder).filePath("workingMergeXML.txt");
    QFile file(newMergeXML);
    file.open(QFile::WriteOnly);
    file.write(replace5.toUtf8());
    file.close();
    QString mergeXMLStatusFile = QDir(outputFolder).filePath("mergeXMLStatus.txt");
    QFile file2(mergeXMLStatusFile);
    file2.open(QFile::WriteOnly);
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

    // run the python in file newMergeXML
    m_mergeScript = newMergeXML;
    runMergeScript();
    QFileInfo workingConfigInfo(workingConfig);
    if (!workingConfigInfo.exists() && !workingConfigInfo.isFile())
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("MergeXML: Unable to create:\n%1").arg(workingConfig));
        QMessageBox::warning(this, tr("Run MergeXML Error"), QString("MergeXML: Unable to create:\n%1").arg(workingConfig));
        return;
    }

    ui->lineEditXMLMasterFile->setText(workingConfig);
    QString newPopulation = QDir(outputFolder).filePath("workingPopulation.txt");
    QFile::copy(lastPopulation, newPopulation);
    ui->lineEditStartingPopulationFile->setText(newPopulation);
    runGA();
}

void MainWindow::runMergeScript()
{
    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Running Merge Script");
    ui->statusBar->showMessage("Running Merge Script");

    QString mergeScriptExecutable = ui->lineEditMergeScriptExecutable->text();
    QFileInfo mergeScriptExecutableInfo(mergeScriptExecutable);
    bool mergeScriptExecutableValid = false;
    if (mergeScriptExecutableInfo.exists() && mergeScriptExecutableInfo.isFile() && mergeScriptExecutableInfo.isExecutable()) mergeScriptExecutableValid = true;
    if (!mergeScriptExecutableValid)
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("runMergeScript: Unable to find \"%1\"").arg(mergeScriptExecutable));
        QMessageBox::warning(this, tr("Run Merge Script Error"), QString("runMergeScript: Unable to find \"%1\"").arg(mergeScriptExecutable));
        return;
    }
    QStringList arguments;
    QProcess mergeScript;
    mergeScript.start(mergeScriptExecutable, arguments);
    if (!mergeScript.waitForStarted())
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("runMergeScript: Unable to start \"%1\"").arg(mergeScriptExecutable));
        QMessageBox::warning(this, tr("Run Merge Script Error"), QString("runMergeScript: Unable to start \"%1\"").arg(mergeScriptExecutable));
        return;
    }
    int msecs = 60 * 60 * 1000;
    if (!mergeScript.waitForFinished(msecs)) // this should work although if mergeScript finished really quickly it is not guaranteed
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("runMergeScript: Timeout \"%1\"").arg(mergeScriptExecutable));
        QMessageBox::warning(this, tr("Run Merge Script Error"), QString("runMergeScript: Timeout \"%1\"").arg(mergeScriptExecutable));
        return;
    }
    QByteArray stdOut = mergeScript.readAllStandardOutput();
    if (ui->spinBoxLogLevel->value() > 1) appendProgress(QString::fromStdString(stdOut.toStdString()));
    QByteArray stdError = mergeScript.readAllStandardError();
    if (ui->spinBoxLogLevel->value() > 1) appendProgress(QString::fromStdString(stdError.toStdString()));
}

void MainWindow::runGaitSym()
{
    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Running GaitSym");
    ui->statusBar->showMessage("Running GaitSym");
    double outputCycle = ui->doubleSpinBoxOutputCycle->value();
    bool cycleFlag = ui->checkBoxCycleTime->isChecked();
    QString outputFolder = ui->lineEditOutputFolder->text();
    QDir dir(outputFolder);
    QStringList files = dir.entryList(QStringList() << "BestGenome*.xml", QDir::Files, QDir::Name);
    if (files.size() < 1)
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("runGaitSym: Unable to find BestGenome*.xml"));
        QMessageBox::warning(this, tr("Run GaitSym Error"), QString("runGaitSym: Unable to find BestGenome*.xml"));
        return;
    }
    QString inputXML = dir.filePath("workingConfig.xml");
    QString outputXML = dir.filePath(files.last());
    QString modelStateFileName = dir.filePath("ModelState.xml");

    QString gaitSymExecutable = ui->lineEditGaitSymExecutable->text();
    QFileInfo gaitSymExecutableInfo(gaitSymExecutable);
    bool gaitSymExecutableValid = false;
    if (gaitSymExecutableInfo.exists() && gaitSymExecutableInfo.isFile() && gaitSymExecutableInfo.isExecutable()) gaitSymExecutableValid = true;
    if (!gaitSymExecutableValid)
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("runGaitSym: Unable to find \"%1\"").arg(gaitSymExecutable));
        QMessageBox::warning(this, tr("Run GaitSym Error"), QString("runGaitSym: Unable to find \"%1\"").arg(gaitSymExecutable));
        return;
    }
    QStringList arguments;
    QString outputCycleArgument;
    if (cycleFlag) outputCycleArgument = "--outputModelStateAtCycle";
    else outputCycleArgument = "--outputModelStateAtTime";
    arguments << "--config" << outputXML
              << outputCycleArgument << QString::number(outputCycle)
              << "--modelState" << modelStateFileName;
    QProcess gaitsym;
    gaitsym.start(gaitSymExecutable, arguments);
    if (!gaitsym.waitForStarted())
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("runGaitSym: Unable to start \"%1\"").arg(gaitSymExecutable));
        QMessageBox::warning(this, tr("Run GaitSym Error"), QString("runGaitSym: Unable to start \"%1\"").arg(gaitSymExecutable));
        return;
    }
    int msecs = 60 * 60 * 1000;
    if (!gaitsym.waitForFinished(msecs)) // this should work although if gaitsym finished really quickly it is not guaranteed
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("runGaitSym: Timeout \"%1\"").arg(gaitSymExecutable));
        QMessageBox::warning(this, tr("Run GaitSym Error"), QString("runGaitSym: Timeout \"%1\"").arg(gaitSymExecutable));
        return;
    }
    QByteArray stdOut = gaitsym.readAllStandardOutput();
    if (ui->spinBoxLogLevel->value() > 1) appendProgress(QString::fromStdString(stdOut.toStdString()));
    QByteArray stdError = gaitsym.readAllStandardError();
    if (ui->spinBoxLogLevel->value() > 1) appendProgress(QString::fromStdString(stdError.toStdString()));
    QFileInfo modelStateInfo(modelStateFileName);
    if (!modelStateInfo.exists() && !modelStateInfo.isFile())
    {
        tryToStopGA();
        m_mergeXMLTimer->stop();
        m_runMergeXML = 0;
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("runGaitSym: Unable to create:\n%1").arg(modelStateFileName));
        QMessageBox::warning(this, tr("Run GaitSym Error"), QString("runGaitSym: Unable to create:\n%1").arg(modelStateFileName));
        return;
    }
}

void MainWindow::pushButtonStopClicked()
{
    if (ui->spinBoxLogLevel->value() > 0) appendProgress("Stop Button Clicked - trying to abort GA");
    ui->statusBar->showMessage("Stop Button Clicked - trying to abort GA");
    tryToStopGA();
    m_mergeXMLTimer->stop();
    m_runMergeXML = 0;
    activateButtons();
}

void MainWindow::handleFinished()
{
    if (!m_ga)
    {
        appendProgress("Error in handleFinished");
        return;
    }
    int result = m_ga->exitCode();
    int exitStatus = m_ga->exitStatus();
    appendProgress(QString::fromStdString(Utilities::toString("handleFinished exitCode = %d exitStatus = %d", result, exitStatus)));
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

void MainWindow::lineEditOutputFolderTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditParameterFileTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditXMLMasterFileTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditStartingPopulationFileTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditModelConfigurationFileTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditModelPopulationFileTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditDriverFileTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditMergeXMLFileTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditWorkingFolderTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditGaitSymExecutableTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditMergeScriptExecutableTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::lineEditGAExecutableTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::spinBoxLogLevelTextChanged(const QString & /*text*/)
{
#if (!defined(WIN32) && ! defined(_WIN32)) || true
    std::string instruction = Utilities::toString("log%d\n", ui->spinBoxLogLevel->value());
    if (m_ga) m_ga->write(instruction.c_str(), instruction.size());
#endif
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::spinBoxTextChanged(const QString & /*text*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::checkBoxStateChanged(int /*state*/)
{
    m_asynchronousGAFileModified = true;
    activateButtons();
}

void MainWindow::activateButtons()
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
        bool mergeScriptExecutableValid = false;
        QFileInfo mergeScriptExecutableInfo(ui->lineEditGaitSymExecutable->text());
        if (mergeScriptExecutableInfo.exists() && mergeScriptExecutableInfo.isFile() && mergeScriptExecutableInfo.isExecutable()) mergeScriptExecutableValid = true;
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

void MainWindow::appendProgress(const QString &text)
{
    ui->plainTextEditLog->appendPlainText(text);
}

void MainWindow::tryToStopGA()
{
    if (m_ga) m_ga->write("stop\n");
    //#if defined(WIN32) || defined(_WIN32)
    //    if (m_ga) m_ga->closeWriteChannel(); // this seems to be needed on Windows otherwise the write channel is never sent
    //#endif
}

void MainWindow::setProgressValue(int value)
{
    ui->progressBar->setValue(value);
}

void MainWindow::setResultNumber(int number)
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 millisecondsDiff = m_lastResultsTime.msecsTo(now);
    if (millisecondsDiff == 0) return;
    ui->lineEditResultNumber->setText(QString::number(number));
    int resultsDiff = number - m_lastResultsNumber;
    m_lastResultsTime = now;
    m_lastResultsNumber = number;
    double generationsPerSecond = 1000.0 * double(resultsDiff) / double(millisecondsDiff);
    ui->lineEditGenerationsPerSecond->setText(QString::number(generationsPerSecond));
}

void MainWindow::setBestScore(double value)
{
    ui->lineEditBestScore->setText(QString::number(value, 'g', 6));
}

void MainWindow::setEvolveID(uint64_t value)
{
    ui->lineEditRunID->setText(QString::number(value));
}

void MainWindow::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "AsynchronousGA4");
    m_asynchronousGAFileName = settings.value("asynchronousGAFileFileName", "").toString();
    m_editorFont.fromString(settings.value("editorFont", QFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)).toString()).toString());
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    setCustomTitle();
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "AnimalSimulationLaboratory", "AsynchronousGA4");
    settings.setValue("asynchronousGAFileFileName", m_asynchronousGAFileName);
    settings.setValue("editorFont", m_editorFont.toString());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.sync();
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    if (m_ga)
    {
        QMessageBox::StandardButton ret = QMessageBox::warning(this, "AsynchronousGA4", "GA is running.\nAre you sure you want to close?.",
                                                               QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No), QMessageBox::No);
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

void MainWindow::createActions()
{
    newAct = new QAction(QIcon(":/Images/new.svg"), tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

    openAct = new QAction(QIcon(":/Images/open.svg"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/Images/save.svg"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);

    saveAsAct = new QAction(QIcon(":/Images/save-as.svg"), tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document to disk with new name"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);

    editSettingsAct = new QAction(QIcon(":/Images/preferences.svg"), tr("Ed&it Settings..."), this);
    editSettingsAct->setShortcuts(QKeySequence::Preferences);
    editSettingsAct->setStatusTip(tr("Open the run specific settings file in an editor"));
    connect(editSettingsAct, &QAction::triggered, this, &MainWindow::editSettings);

    exitAct = new QAction(QIcon(":/Images/exit.svg"), tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &MainWindow::close);

    undoAct = new QAction(QIcon(":/Images/undo.svg"), tr("&Undo"), this);
    undoAct->setShortcuts(QKeySequence::Undo);
    undoAct->setStatusTip(tr("Undo the last operation"));
    connect(undoAct, &QAction::triggered, this, &MainWindow::undo);

    redoAct = new QAction(QIcon(":/Images/redo.svg"), tr("&Redo"), this);
    redoAct->setShortcuts(QKeySequence::Redo);
    redoAct->setStatusTip(tr("Redo the last operation"));
    connect(redoAct, &QAction::triggered, this, &MainWindow::redo);

    cutAct = new QAction(QIcon(":/Images/cut.svg"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, &QAction::triggered, this, &MainWindow::cut);

    copyAct = new QAction(QIcon(":/Images/copy.svg"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, &QAction::triggered, this, &MainWindow::copy);

    pasteAct = new QAction(QIcon(":/Images/paste.svg"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, &QAction::triggered, this, &MainWindow::paste);

    fontAct = new QAction(QIcon(":/Images/font.svg"), tr("&Font"), this);
    fontAct->setStatusTip(tr("Select the font for the text editor windows"));
    connect(fontAct, &QAction::triggered, this, &MainWindow::font);

    aboutAct = new QAction(QIcon(":/Images/icon_design.svg"), tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);
}

void MainWindow::createMenus()
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

void MainWindow::newFile()
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

void MainWindow::open()
{
    if (maybeSave() == false) return;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open AsynchronousGA Private Settings File"), m_asynchronousGAFileName, tr("XML Files (*.xml);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        open(fileName);
    }
}

void MainWindow::open(const QString &fileName)
{
    m_asynchronousGAFileName = fileName;
    m_asynchronousGAFileModified = false;
    m_asynchronousGAFileNameValid = false;
    QFile file(m_asynchronousGAFileName);
    if (file.open(QFile::ReadOnly) == false)
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("open: Unable to open file (read):\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Open File Error"), QString("open: Unable to open file (read):\n%1").arg(m_asynchronousGAFileName));
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QDomDocument doc;
    if (!doc.setContent(data))
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("open: Unable to parse XML file:\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Open File Error"), QString("open: Unable to parse XML file:\n%1").arg(m_asynchronousGAFileName));
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
                ui->lineEditGAExecutable->setText(convertToAbsolutePath(e.attribute("gaExecutable")));
                ui->spinBoxLogLevel->setValue(e.attribute("logLevel").toInt());
                ui->spinBoxPort->setValue(e.attribute("portNumber").toInt());
                ui->doubleSpinBoxStartValue->setValue(e.attribute("startValue").toDouble());
                ui->doubleSpinBoxStepValue->setValue(e.attribute("stepValue").toDouble());
                ui->doubleSpinBoxEndValue->setValue(e.attribute("endValue").toDouble());
                ui->doubleSpinBoxOutputCycle->setValue(e.attribute("outputCycle").toDouble());
                ui->checkBoxMergeXMLActivate->setChecked(QVariant(e.attribute("mergeXMLActivate")).toBool());
                ui->checkBoxCycleTime->setChecked(QVariant(e.attribute("cycle")).toBool());

                m_startExpressionMarker = e.attribute("startExpressionMarker", "[[").toStdString();
                m_endExpressionMarker = e.attribute("endExpressionMarker", "]]").toStdString();
            }
        }
        n = n.nextSibling();
    }
    if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("'%1' read").arg(m_asynchronousGAFileName));
    ui->statusBar->showMessage(QString("'%1' read").arg(m_asynchronousGAFileName));
    m_asynchronousGAFileModified = false;
    m_asynchronousGAFileNameValid = true;
    activateButtons();
    setCustomTitle();
}

void MainWindow::save()
{
    QFile file(m_asynchronousGAFileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("save: Unable to open file (write):\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Save File Error"), QString("save: Unable to open file (write):\n%1").arg(m_asynchronousGAFileName));
        return;
    }

    writeSettings(); // makes sure the settings are all up to date

    QDomDocument doc("AsynchronousGA_Settings_Document_0.1");
    QDomProcessingInstruction  pi = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
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
    dataItemsElement.setAttribute("gaExecutable", convertToRelativePath(ui->lineEditGAExecutable->text()));
    dataItemsElement.setAttribute("logLevel", ui->spinBoxLogLevel->text());
    dataItemsElement.setAttribute("portNumber", ui->spinBoxPort->text());
    dataItemsElement.setAttribute("startValue", ui->doubleSpinBoxStartValue->text());
    dataItemsElement.setAttribute("stepValue", ui->doubleSpinBoxStepValue->text());
    dataItemsElement.setAttribute("endValue", ui->doubleSpinBoxEndValue->text());
    dataItemsElement.setAttribute("outputCycle", ui->doubleSpinBoxOutputCycle->text());
    dataItemsElement.setAttribute("mergeXMLActivate", QString::number(ui->checkBoxMergeXMLActivate->isChecked() ? 1 : 0));
    dataItemsElement.setAttribute("cycle", QString::number(ui->checkBoxCycleTime->isChecked() ? 1 : 0));

    dataItemsElement.setAttribute("startExpressionMarker", QString::fromStdString(m_startExpressionMarker));
    dataItemsElement.setAttribute("endExpressionMarker", QString::fromStdString(m_endExpressionMarker));

    // and now the actual xml doc
    QByteArray xmlData = doc.toByteArray();
    qint64 bytesWritten = file.write(xmlData);
    if (bytesWritten != xmlData.size())
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("save: Unable to write file (write):\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Save File Error"), QString("save: Unable to write file (write):\n%1").arg(m_asynchronousGAFileName));
    }
    file.close();
    if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("'%1' written").arg(m_asynchronousGAFileName));
    ui->statusBar->showMessage(QString("'%1' written").arg(m_asynchronousGAFileName));
    m_asynchronousGAFileModified = false;
    m_asynchronousGAFileNameValid = true;
    activateButtons();
    setCustomTitle();
}

void MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Run Settings File"), m_asynchronousGAFileName, tr("XML Files (*.xml);;Any File (*.* *)"));
    if (!fileName.isEmpty())
    {
        m_asynchronousGAFileName = fileName;
        save();
    }
}

QString MainWindow::convertToRelativePath(const QString &filename)
{
    QFileInfo fileInfo(m_asynchronousGAFileName);
    QDir dir = fileInfo.absoluteDir();
    QString relativePath = QDir::cleanPath(dir.relativeFilePath(filename));
    return relativePath;
}

QString MainWindow::convertToAbsolutePath(const QString &filename)
{
    QFileInfo fileInfo(m_asynchronousGAFileName);
    QDir dir = fileInfo.absoluteDir();
    QString relativePath = QDir::cleanPath(dir.absoluteFilePath(filename));
    return relativePath;
}

void MainWindow::editSettings()
{
    TextEditDialog textEditDialog(this);
    QFile editFile(m_asynchronousGAFileName);
    if (editFile.open(QFile::ReadOnly) == false)
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("editSettings: Unable to open file (read):\n%1").arg(m_asynchronousGAFileName));
        QMessageBox::warning(this, tr("Open File Error"), QString("editSettings: Unable to open file (read):\n%1").arg(m_asynchronousGAFileName));
        return;
    }
    QByteArray editFileData = editFile.readAll();
    editFile.close();
    QString editFileText = QString::fromUtf8(editFileData);

    textEditDialog.useXMLSyntaxHighlighter();
    textEditDialog.setEditorText(editFileText);

    int status = textEditDialog.exec();

    if (status == QDialog::Accepted) // write the new settings
    {
        if (editFile.open(QFile::WriteOnly) == false)
        {
            if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("editSettings: Unable to open file (write):\n%1").arg(m_asynchronousGAFileName));
            QMessageBox::warning(this, tr("Open File Error"), QString("editSettings: Unable to open file (write):\n%1").arg(m_asynchronousGAFileName));
            return;
        }
        editFileData = textEditDialog.editorText().toUtf8();
        editFile.write(editFileData);
        editFile.close();
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("'%1' written").arg(m_asynchronousGAFileName));
        ui->statusBar->showMessage(QString("'%1' written").arg(m_asynchronousGAFileName));
        open(m_asynchronousGAFileName);
    }
    else
    {
        if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("'%1' not changed").arg(m_asynchronousGAFileName));
        ui->statusBar->showMessage(QString("'%1' not changed").arg(m_asynchronousGAFileName));
    }
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(cutAct);
    menu.addAction(copyAct);
    menu.addAction(pasteAct);
    menu.exec(event->globalPos());
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_basicTimer.timerId()) { activateButtons(); }
    else { QMainWindow::timerEvent(event); }
}

void MainWindow::undo()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->undo(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->undo(); return; }
}

void MainWindow::redo()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->redo(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->redo(); return; }
}

void MainWindow::cut()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->cut(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->cut(); return; }
}

void MainWindow::copy()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->copy(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->copy(); return; }
}

void MainWindow::paste()
{
    QWidget *fw = qApp->focusWidget();
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(fw);
    if (lineEdit) { lineEdit->paste(); return; }
    QPlainTextEdit *plainTextEdit = dynamic_cast<QPlainTextEdit *>(fw);
    if (plainTextEdit) { plainTextEdit->paste(); return; }
}

void MainWindow::font()
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

void MainWindow::about()
{
    QString description(u8"This application coordinates optimisation using \nremote clients to do the hard work.\n\u00a9 William Sellers 2022.\nReleased under GPL v3.");
    QString buildDate = QString("Build: %1 %2").arg(__DATE__, __TIME__);
    QString buildType;
#ifdef NDEBUG
    buildType = "Release";
#else
    buildType = "Debug";
#endif
    QString buildInformation = QSysInfo::buildAbi();

    QMessageBox msgBox;
    msgBox.setText(tr("<b>AsynchronousGAQt</b>: Genetic Algorithm Server"));
    msgBox.setInformativeText(QString("%1\n%2 %3\n%4").arg(description, buildDate, buildType, buildInformation));
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

bool MainWindow::maybeSave()
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

void MainWindow::menuRequestPath(QPoint pos)
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
            QString fileName = lineEdit->text();
            if (fileName.endsWith(".xml", Qt::CaseInsensitive) || fileName.endsWith(".gaitsym", Qt::CaseInsensitive)) textEditDialog.useXMLSyntaxHighlighter();
            QFile editFile(fileName);
            if (editFile.open(QFile::ReadOnly) == false)
            {
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("menuRequestPath: Unable to open file (read):\n%1").arg(fileName));
                QMessageBox::warning(this, tr("Open File Error"), QString("menuRequestPath: Unable to open file (read):\n%1").arg(fileName));
                return;
            }
            QByteArray editFileData = editFile.readAll();
            editFile.close();
            QString editFileText = QString::fromUtf8(editFileData);

            textEditDialog.setEditorText(editFileText);

            int status = textEditDialog.exec();

            if (status == QDialog::Accepted) // write the new settings
            {
                if (editFile.open(QFile::WriteOnly) == false)
                {
                    if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("menuRequestPath: Unable to open file (write):\n%1").arg(fileName));
                    QMessageBox::warning(this, tr("Open File Error"), QString("menuRequestPath: Unable to open file (write):\n%1").arg(fileName));
                    return;
                }
                editFileData = textEditDialog.editorText().toUtf8();
                editFile.write(editFileData);
                editFile.close();
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("'%1' written").arg(lineEdit->text()));
                ui->statusBar->showMessage(QString("'%1' written").arg(lineEdit->text()));
            }
            else
            {
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("'%1' not changed").arg(lineEdit->text()));
                ui->statusBar->showMessage(QString("'%1' not changed").arg(lineEdit->text()));
            }
            break;
        }
        if (action->text() == tr("Edit File...") && sender() == ui->lineEditParameterFile)
        {

            GAParametersDialog gaParametersDialog(this);
            QString fileName = lineEdit->text();
            QFile editFile(fileName);
            if (editFile.open(QFile::ReadOnly) == false)
            {
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("menuRequestPath: Unable to open file (read):\n%1").arg(fileName));
                QMessageBox::warning(this, tr("Open File Error"), QString("menuRequestPath: Unable to open file (read):\n%1").arg(fileName));
                return;
            }
            QByteArray editFileData = editFile.readAll();
            editFile.close();
            QString editFileText = QString::fromUtf8(editFileData);

            gaParametersDialog.setEditorText(editFileText);

            int status = gaParametersDialog.exec();

            if (status == QDialog::Accepted) // write the new settings
            {
                if (editFile.open(QFile::WriteOnly) == false)
                {
                    if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("menuRequestPath: Unable to open file (write):\n%1").arg(fileName));
                    QMessageBox::warning(this, tr("Open File Error"), QString("menuRequestPath: Unable to open file (write):\n%1").arg(fileName));
                    return;
                }
                editFileData = gaParametersDialog.editorText().toUtf8();
                editFile.write(editFileData);
                editFile.close();
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("'%1' written").arg(lineEdit->text()));
                ui->statusBar->showMessage(QString("'%1' written").arg(lineEdit->text()));
            }
            else
            {
                if (ui->spinBoxLogLevel->value() > 0) appendProgress(QString("'%1' not changed").arg(lineEdit->text()));
                ui->statusBar->showMessage(QString("'%1' not changed").arg(lineEdit->text()));
            }
            break;
        }
        break;
    }
    delete menu;
}

void MainWindow::setEditorFonts()
{
    QList<QPlainTextEdit *> listQPlainTextEdit = this->findChildren<QPlainTextEdit *>(QRegularExpression(".*", QRegularExpression::CaseInsensitiveOption), Qt::FindChildrenRecursively);
    for (QList<QPlainTextEdit *>::iterator it = listQPlainTextEdit.begin(); it != listQPlainTextEdit.end(); it++) (*it)->setFont(m_editorFont);
}

void MainWindow::setCustomTitle()
{
    QFileInfo fileInfo(m_asynchronousGAFileName);
    QString windowTitle = fileInfo.absoluteFilePath();
    if (m_asynchronousGAFileModified) windowTitle += "*";
    if (!m_asynchronousGAFileNameValid) windowTitle += "+";
    this->setWindowTitle(windowTitle);
}

void MainWindow::readStandardError()
{
    QString output = m_ga->readAllStandardError();
    static QRegularExpression re("\\n|\\r");
    QStringList lines = output.split(re, Qt::SkipEmptyParts);
    for (auto &&it : lines)
    {
        QStringList tokens = it.split('=');
        if (tokens.size() == 2)
        {
            QString code = tokens[0].trimmed();
            if (code == "Progress") { setProgressValue(tokens[1].toInt()); continue; }
            if (code == "Return Count") { setResultNumber(tokens[1].toInt()); continue; }
            if (code == "Best Score") { setBestScore(tokens[1].toDouble()); continue; }
            if (code == "Evolve Identifier") { setEvolveID(tokens[1].toULongLong()); continue; }
        }
        appendProgress(it + '\n');
    }
}

void MainWindow::readStandardOutput()
{
    QString output = m_ga->readAllStandardOutput();
    static QRegularExpression re("\\n|\\r");
    QStringList lines = output.split(re, Qt::SkipEmptyParts);
    appendProgress(lines.join('\n'));
}

