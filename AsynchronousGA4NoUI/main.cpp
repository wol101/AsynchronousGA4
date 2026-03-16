#include "ArgParse.h"
#include "Wrapper.h"

#include "pystring.h"

#include <iostream>

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
    argparse.AddArgument("-l"s, "--logLevel"s, "0, 1, 2 outputs more detail with higher numbers [0]"s, ""s, 1, false, ArgParse::Int);

    int err = argparse.Parse();
    if (err)
    {
        argparse.Usage();
        exit(1);
    }

    int logLevel;
    std::string settingsFile;
    bool logLevelSet = argparse.Get("--logLevel"s, &logLevel);
    argparse.Get("--settingsFile"s, &settingsFile);
    if (logLevel > 1) std::cerr << pystring::join("\n"s, argparse.rawArguments()) << "\n";;

    Wrapper wrapper;
    if (logLevelSet) wrapper.setLogLevel(logLevel);
    wrapper.openSettingsFile(settingsFile);
    wrapper.run();
}
