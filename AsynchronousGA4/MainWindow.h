#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QTimer>
#include <QDateTime>
#include <QProcess>

#include <sstream>
#include <iostream>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow() override;

public slots:
    void pushButtonParameterFileClicked();
    void pushButtonStartingPopulationFileClicked();
    void pushButtonXMLMasterFileClicked();
    void pushButtonOutputFolderClicked();
    void pushButtonModelConfigurationFileClicked();
    void pushButtonModelPopulationFileClicked();
    void pushButtonDriverFileClicked();
    void pushButtonWorkingFolderClicked();
    void pushButtonMergeXMLFileClicked();
    void pushButtonGaitSymExecutableClicked();
    void pushButtonMergeScriptExecutableClicked();
    void pushButtonGAExecutableClicked();
    void pushButtonStartClicked();
    void pushButtonStopClicked();
    void checkBoxMergeXMLActivateClicked();
    void handleFinished();
    void lineEditOutputFolderTextChanged(const QString &text);
    void lineEditParameterFileTextChanged(const QString &text);
    void lineEditXMLMasterFileTextChanged(const QString &text);
    void lineEditStartingPopulationFileTextChanged(const QString &text);
    void lineEditModelConfigurationFileTextChanged(const QString &text);
    void lineEditModelPopulationFileTextChanged(const QString &text);
    void lineEditDriverFileTextChanged(const QString &text);
    void lineEditMergeXMLFileTextChanged(const QString &text);
    void lineEditWorkingFolderTextChanged(const QString &text);
    void lineEditGaitSymExecutableTextChanged(const QString &text);
    void lineEditMergeScriptExecutableTextChanged(const QString &text);
    void lineEditGAExecutableTextChanged(const QString &text);
    void checkBoxStateChanged(int state);
    void spinBoxLogLevelTextChanged(const QString &text);
    void spinBoxTextChanged(const QString &text);
    void appendProgress(const QString &text);
    void setProgressValue(int value);
    void setResultNumber(int number);
    void setBestScore(double value);
    void setEvolveID(uint64_t value);
    void tryToStopGA();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void editSettings();
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void font();
    void about();

    void runMergeXML();
    void menuRequestPath(QPoint pos);

    void readStandardError();
    void readStandardOutput();

private:
    void closeEvent(QCloseEvent *event) override;
    void setCustomTitle();
    void activateButtons();
    QString convertToRelativePath(const QString &filename);
    QString convertToAbsolutePath(const QString &filename);

    Ui::MainWindow *ui = nullptr;
    QProcess *m_ga = nullptr;
    QTimer *m_mergeXMLTimer = nullptr;
    int m_runMergeXML = 0;
    double m_currentLoopValue = 0;
    int m_currentLoopCount = 0;
    QDateTime m_lastResultsTime;
    int m_lastResultsNumber = -1;

    bool m_asynchronousGAFileModified = false;
    QString m_asynchronousGAFileName;
    bool m_asynchronousGAFileNameValid = false;

    std::string m_outputFolder;
    std::string m_parameterFile;
    std::string m_xmlMasterFile;
    std::string m_startingPopulationFile;

    std::string m_startExpressionMarker = {"[["};
    std::string m_endExpressionMarker = {"]]"};

    QString m_mergeScript;
    void runMergeScript();

    void createActions();
    void createMenus();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    void runGA();
    void runGaitSym();
    void open(const QString &fileName);
    void setEditorFonts();

    std::stringstream m_capturedCerr;
    std::unique_ptr<cerrRedirect> m_redirect;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *helpMenu;
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *editSettingsAct;
    QAction *exitAct;
    QAction *undoAct;
    QAction *redoAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *fontAct;
    QAction *aboutAct;

    QFont m_editorFont;
    QBasicTimer m_basicTimer;
};

#endif // MAINWINDOW_H
