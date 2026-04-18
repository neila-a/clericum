/**
 * @mainpage Clericum
 *
 * @brief 基于 FUSE 的文件备份和虚拟文件系统工具
 *
 * Clericum 提供以下功能：
 * - 创建带有元数据的 store 文件夹
 * - 通过 FUSE 挂载虚拟文件系统
 * - 备份和恢复文件
 *
 * @section commands 命令
 *
 * - create <path>: 创建新的 store 文件夹
 * - load <store> <path>: 挂载 store 到指定路径
 * - unload <path>: 卸载挂载点
 * - backup <path> <name>: 创建备份
 * - gui: 启动图形界面
 *
 * @section structure Store 结构
 *
 * @code
 * store/
 * ├── .clericum-store   # 标记
 * ├── filename/
 * │   ├── current        # 本源文件
 * │   └── backups/
 * │       └── backup1
 * @endcode
 *
 * @author Clericum Team
 * @version _PROJECT_VERSION
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTimer>
#include <csignal>
#include <unistd.h>

#include "commandhandler.h"

// 全局命令处理器指针，用于信号处理
static CommandHandler *g_handler = nullptr;

// 信号处理函数
static void signalHandler(int signum)
{
    qInfo() << "Received signal" << signum << ", shutting down...";
    QCoreApplication::quit();
}

/**
 * @brief 主函数
 *
 * 处理命令行参数并执行相应命令。
 */
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    app.setApplicationName(_PROJECT_NAME);
    app.setApplicationVersion(_PROJECT_VERSION);
    app.setOrganizationName(_PROJECT_NAME);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("%1 - A FUSE-based file backup and virtual filesystem tool\n\n"
                       "Commands:\n"
                       "  create <path>         Create a new store folder\n"
                       "  load <store> <path>   Mount store to path\n"
                       "  unload <path>         Unmount filesystem\n"
                       "  backup <path> <name>  Create backup of file\n"
                       "  gui                   Launch GUI (not implemented)")
        .arg(_PROJECT_NAME)
    );

    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("action", "The action to perform: create, load, unload, backup, or gui");

    parser.process(app);

    const QStringList arguments = parser.positionalArguments();
    if (arguments.isEmpty()) {
        parser.showHelp(-1);
        return -1;
    }

    const QString action = arguments.first();

    // 创建命令处理器
    CommandHandler handler;
    g_handler = &handler;

    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    CommandHandler::Result result;
    bool shouldRunEventLoop = false;

    if (action == QStringLiteral("gui")) {
        // GUI 模式（暂未实现）
        qCritical() << "GUI mode is not yet implemented";
        return -1;

    } else if (action == QStringLiteral("create")) {
        // create <path>
        if (arguments.length() < 2) {
            qCritical() << "create requires a path argument";
            parser.showHelp(-1);
        }
        const QString path = arguments.at(1);
        result = handler.executeCreate(path);

    } else if (action == QStringLiteral("load")) {
        // load <store> <path>
        if (arguments.length() < 3) {
            qCritical() << "load requires store and path arguments";
            parser.showHelp(-1);
        }
        const QString store = arguments.at(1);
        const QString path = arguments.at(2);
        result = handler.executeLoad(store, path);

        // load 命令需要保持运行
        if (result.success) {
            shouldRunEventLoop = true;
        }

    } else if (action == QStringLiteral("unload")) {
        // unload <path>
        if (arguments.length() < 2) {
            qCritical() << "unload requires a path argument";
            parser.showHelp(-1);
        }
        const QString path = arguments.at(1);
        result = handler.executeUnload(path);

    } else if (action == QStringLiteral("backup")) {
        // backup <path> <name>
        if (arguments.length() < 3) {
            qCritical() << "backup requires path and name arguments";
            parser.showHelp(-1);
        }
        const QString path = arguments.at(1);
        const QString name = arguments.at(2);
        result = handler.executeBackup(path, name);

    } else {
        qCritical() << "Unknown action:" << action;
        parser.showHelp(-1);
    }

    // 输出结果
    if (result.success) {
        if (!result.message.isEmpty()) {
            qInfo() << result.message;
        }
        if (shouldRunEventLoop) {
            // 进入事件循环保持运行
            return app.exec();
        }
        return 0;
    } else {
        if (!result.error.isEmpty()) {
            qCritical() << "Error:" << result.message << result.error;
        }
        return -1;
    }
}
