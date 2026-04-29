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

#include <KAboutData>
#include "commandparser.h"

 /**
  * @brief 主函数
  *
  * 处理命令行参数并执行相应命令。
  */
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    KLocalizedString::setApplicationDomain(_PROJECT_NAME);

    KAboutData aboutData(
        QStringLiteral(_PROJECT_NAME),
        QStringLiteral(_PROJECT_NAME),
        QStringLiteral(_PROJECT_VERSION),
        i18n("description"),
        KAboutLicense::GPL_V3,
        QStringLiteral("Copyright (C) 2026 Neila"),
        QString(),
        QStringLiteral(_PROJECT_HOMEPAGE),
        QStringLiteral(_PROJECT_HOMEPAGE "/issues")
    );
    aboutData.addAuthor(
        QStringLiteral("Neila"),
        QStringLiteral("any"),
        QStringLiteral("neilaspace@outlook.com"),
        QStringLiteral("https://neilasite.pages.dev"),
        QStringLiteral("https://avatars.githubusercontent.com/u/78797625")
    );
    KAboutData::setApplicationData(aboutData);

    CommandParser parser;
    aboutData.setupCommandLine(&parser);

    CommandHandler handler;
#define executor(command) [&handler](QStringList arguments){ return handler.execute##command; }
    parser

        // store 相关命令
        .registerCommand(
            { "store", "create" },
            { i18n("path") },
            i18n("Create a new store"),
            executor(Create(arguments[0]))
        )
        .registerCommand(
            { "store", "load" },
            { i18n("store"), i18n("path") },
            i18n("Mount a store to path"),
            executor(Load(arguments[0], arguments[1]))
        )
        .registerCommand(
            { "store", "unload" },
            { i18n("path") },
            i18n("Unmount a filesystem"),
            executor(Unload(arguments[0]))
        )

        // backup 相关命令
        .registerCommand(
            { "backup", "create" },
            { i18n("path"), i18n("name") },
            i18n("Create backup of file"),
            executor(Backup(arguments[0], arguments[1]))
        )
        .registerCommand(
            { "backup", "load" },
            { i18n("path"), i18n("name") },
            i18n("Load a backup file"),
            executor(BackupLoad(arguments[0], arguments[1]))
        )

        ;
#undef executor

    parser.setApplication(aboutData.shortDescription());
    return parser.process(app);
}
