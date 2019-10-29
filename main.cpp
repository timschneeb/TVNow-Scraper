#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include "scraper.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("TVNow Scraper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("id", "Series ID");

    /*
    QCommandLineOption targetDirectoryOption(QStringList() << "t" << "target-directory",
            "Copy all source files into <directory>.",
            "main", "directory");
    parser.addOption(targetDirectoryOption);*/

    QCommandLineOption forceOption(QStringList() << "a" << "all",
            QCoreApplication::translate("main", "Downloads all available qualities (instead of the best one)"));
    parser.addOption(forceOption);

    // Process the actual command line arguments given by the user
    parser.process(a);

    const QStringList args = parser.positionalArguments();
    if(args.count()<=0){
        qStdErr() << "Error: No Series ID set" << endl;
        return 0;
    }else if(args.count()>1){
        qStdErr() << "Error: Too many arguments" << endl;
        return 0;
    }
    Scraper* scraper = new Scraper();
    scraper->updateIndex(args.at(0),parser.isSet(forceOption));
    scraper->begin();

    //QString targetDir = parser.value(targetDirectoryOption);

    exit(0);
    return a.exec();
}
