/**
 * @file fileitemaction.cpp
 * @brief ClericumFileItemAction 类实现
 */

 // 插件自身的翻译域，使用项目名（与 CLI / catalog 一致）。
 // 必须在包含任何 KDE i18n 头文件之前定义。
#define TRANSLATION_DOMAIN _PROJECT_NAME

#include "fileitemaction.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QList>

// 在引入 storemanager.h 之前已完成所有外部头文件包含，避免其全局日志宏
// （log/critical/information/warn）污染后续头文件。
#include "storemanager.h"
#include "clericumfuse.h"
#include "commandhandler.h"

// 本文件只用到 StoreManager / CommandHandler 的 API，不需要这些日志宏；
// 它们在后续头文件中可能引发命名冲突，故在此全部取消。
#undef log
#undef critical
#undef information
#undef warn

K_PLUGIN_CLASS_WITH_JSON(ClericumFileItemAction, "fileitemaction.json")

ClericumFileItemAction::ClericumFileItemAction(QObject* parent,
    const QVariantList& args)
    : KAbstractFileItemActionPlugin(parent) {
    Q_UNUSED(args);
}

ClericumFileItemAction::~ClericumFileItemAction() {
}

QList<QAction*> ClericumFileItemAction::actions(
    const KFileItemListProperties& fileItemInfos, QWidget* parentWidget) {
    // 仅支持本地文件、且只处理单个选中项
    if (!fileItemInfos.isLocal()) {
        return {};
    }

    const QList<QUrl> urls = fileItemInfos.urlList();
    if (urls.size() != 1) {
        return {};
    }

    const QString path = urls.at(0).toLocalFile();
    if (path.isEmpty()) {
        return {};
    }

    const QFileInfo info(path);
    const bool isDir = info.isDir();
    CommandHandler handler;
    const CommandHandler::MountInfo mount = handler.findMountPoint(path);
    const bool isStore = isDir
        && QDir(path).exists(StoreManager::METADATA_FILENAME);
    const bool isMountPoint = isDir
        && QDir(path).exists(MOUNT_MARKER_FILE);

    QList<QAction*> actions;

    // 在任意普通（非 store、非挂载点）文件夹上：提供创建 store 与挂载操作。
    if (isDir && !isStore && !isMountPoint) {
        QAction* createStoreAction = new QAction(
            QIcon::fromTheme("folder-new"),
            i18n("Create store…"),
            parentWidget
        );
        connect(createStoreAction, &QAction::triggered, this,
            [this, path]() {
                CommandHandler().executeCreate(path);
            });
        actions.append(createStoreAction);

        QAction* loadAction = new QAction(
            QIcon::fromTheme("media-mount"), i18n("Load…"), parentWidget);
        connect(loadAction, &QAction::triggered, this,
            [this, path, parentWidget]() {
                // 当前文件夹作为挂载点，弹出目录选择框以选择要挂载的 store。
                const QString storePath = QFileDialog::getExistingDirectory(parentWidget,
                    i18n("Select store to mount into %1", path), path);
                if (storePath.isEmpty()) {
                    return;
                }
                CommandHandler().executeLoad(storePath, path);
            });
        actions.append(loadAction);
    } else if (isMountPoint) {
        // 在挂载点上：提供卸载操作
        QAction* unloadAction = new QAction(
            QIcon::fromTheme("media-eject"),
            i18n("Unload"),
            parentWidget
        );
        connect(unloadAction, &QAction::triggered, this,
            [this, path]() {
                CommandHandler().executeUnload(path);
            });
        actions.append(unloadAction);
    } else if (!mount.storePath.isEmpty() && !isDir) {
        // 在挂载点内的文件上：提供备份相关操作
        const QString fileName = info.fileName();
        StoreManager manager;
        manager.setStorePath(mount.storePath);
        const QString virtualPath = QDir(mount.mountPath).filePath(fileName);

        if (manager.isBackupFile(fileName)) {
            QAction* loadBackupAction = new QAction(
                QIcon::fromTheme("document-open"),
                i18n("Load backup"),
                parentWidget
            );
            connect(loadBackupAction, &QAction::triggered, this,
                [this, virtualPath]() {
                    CommandHandler().executeBackupLoad(virtualPath);
                });
            actions.append(loadBackupAction);

            QAction* removeBackupAction = new QAction(
                QIcon::fromTheme("delete"),
                i18n("Remove backup"),
                parentWidget
            );
            connect(removeBackupAction, &QAction::triggered, this,
                [this, virtualPath]() {
                    CommandHandler().executeBackupRemove(virtualPath);
                });
            actions.append(removeBackupAction);
        } else {
            QAction* createBackupAction = new QAction(
                QIcon::fromTheme("backup"),
                i18n("Create backup…"),
                parentWidget
            );
            connect(createBackupAction, &QAction::triggered, this,
                [this, virtualPath, parentWidget]() {
                    bool ok = false;
                    const QString name = QInputDialog::getText(parentWidget,
                        i18n("Create backup"), i18n("Backup name:"),
                        QLineEdit::Normal, QString(), &ok);
                    if (!ok || name.isEmpty()) {
                        return;
                    }
                    CommandHandler().executeBackup(virtualPath, name);
                });
            actions.append(createBackupAction);
        }
    }
            QAction* action = new QAction(
                QIcon::fromTheme("backup"),
                i18n("Create backup…"),
                parentWidget
            );
            actions.append(action);

    return actions;
}

#include "fileitemaction.moc"
