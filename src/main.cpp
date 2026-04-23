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
 * - store create <path>: 创建新的 store 文件夹
 * - store load <store> <path>: 挂载 store 到指定路径
 * - store unload <path>: 卸载挂载点
 * - backup create <path> <name>: 创建备份
 * - backup load <path> <name>: 从备份加载到本源文件
 * - gui: 启动图形界面
 *
 * @section structure Store 结构
 *
 * ```
 * store/
 * ├── .clericum-store   # 标记
 * ├── files/
 * │   ├── filename1     # 本源文件
 * │   └── filename2
 * └── backups/
 *     ├── filename1/
 *     │   └── backup1
 *     └── filename2/
 *         └── backup3
 * ```
 *
 * @author Neila
 * @version _PROJECT_VERSION
 */

#include <QCommandLineParser>
#include <csignal>

#include <KAboutData>

#include "commandhandler.h"

/**
 * @brief 主函数
 *
 * 处理命令行参数并执行相应命令。
 */
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    KAboutData aboutData(
        QStringLiteral(_PROJECT_NAME),
        QStringLiteral(_PROJECT_NAME),
        QStringLiteral(_PROJECT_VERSION),
        QStringLiteral(_PROJECT_DESCRIPTION),
        KAboutLicense::GPL_V3,
        QStringLiteral("Copyright (C) 2026 Neila"),
        QString(),
        QStringLiteral(_PROJECT_HOMEPAGE),
        QStringLiteral(_PROJECT_HOMEPAGE "/issues")
    );
    aboutData.addAuthor("Neila", "any", "neilaspace@outlook.com", "https://neilasite.pages.dev", "https://avatars.githubusercontent.com/u/78797625");
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.setApplicationDescription(aboutData.shortDescription() + "\n"
        "Commands:\n"
        "  store create <path>     Create a new store folder\n"
        "  store load <store> <path>   Mount store to path\n"
        "  store unload <path>    Unmount filesystem\n"
        "  backup create <path> <name>  Create backup of file\n"
        "  backup load <path> <name>   Load a backup file\n"
        "  gui                   Launch GUI (not implemented)");

    parser.addPositionalArgument("action", "The action to perform: store, backup, or gui");
    parser.addPositionalArgument("subaction", "Sub action: create, load, unload (for store); create, load (for backup)");
    parser.addPositionalArgument("args", "Additional arguments");

    parser.process(app);

    const QStringList arguments = parser.positionalArguments();
    if (arguments.isEmpty()) {
        parser.showHelp(-1);
        return -1;
    }

    const QString action = arguments.first();

    // 创建命令处理器
    CommandHandler handler;

    CommandHandler::Result result;

    if (action == QStringLiteral("gui")) {
        // GUI 模式（暂未实现）
        qCritical() << "GUI mode is not yet implemented";
        return -1;

    } else if (action == QStringLiteral("store")) {
        // store create <path>
        // store load <store> <path>
        // store unload <path>
        if (arguments.length() < 2) {
            qCritical() << "store requires a subcommand (create, load, unload)";
            parser.showHelp(-1);
        }
        const QString subaction = arguments.at(1);

        if (subaction == QStringLiteral("create")) {
            if (arguments.length() < 3) {
                qCritical() << "store create requires a path argument";
                parser.showHelp(-1);
            }
            const QString path = arguments.at(2);
            result = handler.executeCreate(path);

        } else if (subaction == QStringLiteral("load")) {
            if (arguments.length() < 4) {
                qCritical() << "store load requires store and path arguments";
                parser.showHelp(-1);
            }
            const QString store = arguments.at(2);
            const QString path = arguments.at(3);
            result = handler.executeLoad(store, path);
        } else if (subaction == QStringLiteral("unload")) {
            if (arguments.length() < 3) {
                qCritical() << "store unload requires a path argument";
                parser.showHelp(-1);
            }
            const QString path = arguments.at(2);
            result = handler.executeUnload(path);
        } else {
            qCritical() << "Unknown store subcommand:" << subaction;
            parser.showHelp(-1);
        }

    } else if (action == QStringLiteral("backup")) {
        // backup create <path> <name>
        // backup load <path> <name>
        if (arguments.length() < 2) {
            qCritical() << "backup requires a subcommand (create, load)";
            parser.showHelp(-1);
        }
        const QString subaction = arguments.at(1);

        if (subaction == QStringLiteral("create")) {
            if (arguments.length() < 4) {
                qCritical() << "backup create requires path and name arguments";
                parser.showHelp(-1);
            }
            const QString path = arguments.at(2);
            const QString name = arguments.at(3);
            result = handler.executeBackup(path, name);

        } else if (subaction == QStringLiteral("load")) {
            if (arguments.length() < 4) {
                qCritical() << "backup load requires path and name arguments";
                parser.showHelp(-1);
            }
            const QString path = arguments.at(2);
            const QString name = arguments.at(3);
            result = handler.executeBackupLoad(path, name);

        } else {
            qCritical() << "Unknown backup subcommand:" << subaction;
            parser.showHelp(-1);
        }

    } else {
        qCritical() << "Unknown action:" << action;
        parser.showHelp(-1);
    }

    // 输出结果
    if (result.success) {
        if (!result.message.isEmpty()) {
            qInfo() << result.message;
        }
        return 0;
    } else {
        if (!result.error.isEmpty()) {
            qCritical() << "Error:" << result.message << result.error;
        }
        return -1;
    }
}
