#include "ArgParse.h"
#include "Wrapper.h"

using namespace std::string_literals;

int main(int argc, const char **argv)
{
    std::string compileDate(__DATE__);
    std::string compileTime(__TIME__);
    ArgParse argparse;
    argparse.Initialise(argc, argv, "AsynchronousGA4NoUI distributed genetic algorithm program "s + compileDate + " "s + compileTime, 0, 0);
    // required arguments
    argparse.AddArgument("-s"s, "--settingsFile"s, "Settings XML file specifying the run"s, ""s, 1, true, ArgParse::String);
    // optional arguments
    argparse.AddArgument("-l"s, "--logLevel"s, "0, 1, 2 outputs more detail with higher numbers [0]"s, "0"s, 1, false, ArgParse::Int);

    int err = argparse.Parse();
    if (err)
    {
        argparse.Usage();
        exit(1);
    }

    bool logLevelSet;
    int logLevel;
    std::string settingsFile;
    logLevelSet = argparse.Get("--logLevel"s, &logLevel);
    argparse.Get("--settingsFile"s, &settingsFile);

    Wrapper wrapper(settingsFile);
    if (logLevelSet) wrapper.setLogLevel(logLevel);
    wrapper.run();
}
