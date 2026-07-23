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
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QList>
#include <QMessageBox>

// 在引入 storemanager.h 之前已完成所有外部头文件包含，避免其全局日志宏
// （log/critical/information/warn）污染后续头文件（如 QMessageBox）。
#include "storemanager.h"
#include "clericumfuse.h"
#include "commandhandler.h"

// 本文件只用到 StoreManager / CommandHandler 的 API，不需要这些日志宏；
// 且它们与 QMessageBox 的同名静态方法冲突，故在此全部取消。
#undef log
#undef critical
#undef information
#undef warn

namespace {

// 将 CommandHandler 的执行结果反馈给用户（成功消息或错误）。
void reportResult(const CommandHandler::Result& result, QWidget* parentWidget) {
    if (result.has_value()) {
        if (!result->isEmpty()) {
            QMessageBox::information(parentWidget, QStringLiteral(_PROJECT_NAME), *result);
        }
    } else {
        QMessageBox::critical(parentWidget, QStringLiteral(_PROJECT_NAME), result.error());
    }
}

}

K_PLUGIN_CLASS_WITH_JSON(ClericumFileItemAction, "fileitemaction.json")

ClericumFileItemAction::ClericumFileItemAction(QObject* parent,
    const QVariantList& args)
    : KAbstractFileItemActionPlugin(parent) {
    Q_UNUSED(args);
}

ClericumFileItemAction::~ClericumFileItemAction() {
}

ClericumFileItemAction::MountInfo ClericumFileItemAction::findMountPoint(
    const QString& path) const {
    MountInfo info;

    const QFileInfo fileInfo(path);
    QDir dir = fileInfo.isFile() ? fileInfo.absoluteDir() : QDir(path);

    while (!dir.isRoot()) {
        const QString markerFile = dir.filePath(MOUNT_MARKER_FILE);
        if (QFile::exists(markerFile)) {
            info.mountPath = dir.absolutePath();
            QFile file(markerFile);
            if (file.open(QIODevice::ReadOnly)) {
                info.storePath = QString::fromUtf8(file.readAll()).trimmed();
                file.close();
            }
            break;
        }
        if (!dir.cdUp()) {
            break;
        }
    }

    return info;
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
    const MountInfo mount = findMountPoint(path);
    const bool isStore = isDir
        && QDir(path).exists(StoreManager::METADATA_FILENAME);
    const bool isMountPoint = isDir
        && QDir(path).exists(MOUNT_MARKER_FILE);

    QList<QAction*> result;

    // 在 store 文件夹上：提供挂载操作
    if (isStore && !isMountPoint && mount.mountPath.isEmpty()) {
        QAction* loadAction = new QAction(
            QIcon::fromTheme("media-mount"), i18n("Load…"), parentWidget);
        connect(loadAction, &QAction::triggered, this,
            [this, path, parentWidget]() { loadStore(path, parentWidget); });
        result.append(loadAction);
    } else if (isMountPoint) {
        // 在挂载点上：提供卸载操作
        QAction* unloadAction = new QAction(
            QIcon::fromTheme("media-eject"), i18n("Unload"), parentWidget);
        connect(unloadAction, &QAction::triggered, this,
            [this, path, parentWidget]() { unloadMount(path, parentWidget); });
        result.append(unloadAction);
    }

    // 在挂载点内的文件上：提供备份相关操作
    if (!mount.storePath.isEmpty() && !isDir) {
        const QString fileName = info.fileName();
        StoreManager manager;
        manager.setStorePath(mount.storePath);
        const QString virtualPath = QDir(mount.mountPath).filePath(fileName);

        if (manager.isBackupFile(fileName)) {
            QAction* loadBackupAction = new QAction(
                QIcon::fromTheme("document-open"),
                i18n("Load backup"), parentWidget);
            connect(loadBackupAction, &QAction::triggered, this,
                [this, virtualPath, parentWidget]() {
                    CommandHandler handler;
                    reportResult(handler.executeBackupLoad(virtualPath), parentWidget);
                });
            result.append(loadBackupAction);

            QAction* removeBackupAction = new QAction(
                QIcon::fromTheme("delete"),
                i18n("Remove backup"), parentWidget);
            connect(removeBackupAction, &QAction::triggered, this,
                [this, virtualPath, parentWidget]() {
                    CommandHandler handler;
                    reportResult(handler.executeBackupRemove(virtualPath), parentWidget);
                });
            result.append(removeBackupAction);
        } else {
            QAction* createBackupAction = new QAction(
                QIcon::fromTheme("backup"),
                i18n("Create backup…"), parentWidget);
            connect(createBackupAction, &QAction::triggered, this,
                [this, virtualPath, parentWidget]() {
                    createBackup(virtualPath, parentWidget);
                });
            result.append(createBackupAction);
        }
    }

    return result;
}

void ClericumFileItemAction::loadStore(const QString& storePath,
    QWidget* parentWidget) {
    const QString defaultDir = QFileInfo(storePath).absoluteDir().absolutePath();
    const QString mountPath = QFileDialog::getExistingDirectory(parentWidget,
        i18n("Select mount point for %1", storePath), defaultDir);
    if (mountPath.isEmpty()) {
        return;
    }
    CommandHandler handler;
    reportResult(handler.executeLoad(storePath, mountPath), parentWidget);
}

void ClericumFileItemAction::unloadMount(const QString& mountPath,
    QWidget* parentWidget) {
    CommandHandler handler;
    reportResult(handler.executeUnload(mountPath), parentWidget);
}

void ClericumFileItemAction::createBackup(const QString& virtualPath,
    QWidget* parentWidget) {
    bool ok = false;
    const QString name = QInputDialog::getText(parentWidget,
        i18n("Create backup"), i18n("Backup name:"),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || name.isEmpty()) {
        return;
    }
    CommandHandler handler;
    reportResult(handler.executeBackup(virtualPath, name), parentWidget);
}

#include "fileitemaction.moc"
