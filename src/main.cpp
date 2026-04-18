#include <QGuiApplication>
#include <QQmlApplicationEngine>
#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <QCommandLineParser>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    app.setApplicationName(_PROJECT_NAME);
    app.setApplicationVersion(_PROJECT_VERSION);
    parser.setApplicationDescription(_PROJECT_DESCRIPTION);

    parser.addPositionalArgument("action", "The action to do");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(app);
    const QStringList arguments = parser.positionalArguments();
    if (arguments.length() > 0) {
        const QString action = arguments[0];
        if (action.compare("gui") == 0) {
            // gui 命令处理
            QQmlApplicationEngine engine;
            QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                &app, []() { QCoreApplication::exit(-1); },
                Qt::QueuedConnection);
            engine.loadFromModule(_PROJECT_NAME, "Main");

            return QCoreApplication::exec();
        } else if (action.compare("create") == 0) {
            // create <path> 命令处理
            if (arguments.length() < 2) {
                parser.showHelp(-1);
            }
            const QString path = arguments[1];
            // TODO: 实现 create 功能
        } else if (action.compare("load") == 0) {
            // load <store> <path> 命令处理
            if (arguments.length() < 3) {
                parser.showHelp(-1);
            }
            const QString store = arguments[1];
            const QString path = arguments[2];
            // TODO: 实现 load 功能
        } else if (action.compare("unload") == 0) {
            // unload <path> 命令处理
            if (arguments.length() < 2) {
                parser.showHelp(-1);
            }
            const QString path = arguments[1];
            // TODO: 实现 unload 功能
        } else if (action.compare("backup") == 0) {
            // backup <path> 命令处理
            if (arguments.length() < 2) {
                parser.showHelp(-1);
            }
            const QString path = arguments[1];
            // TODO: 实现 backup 功能
        }
    }
    return 0;
}
