/**
 * @file commandhandler.cpp
 * @brief CommandHandler 类实现
 */

#include "commandhandler.h"
#include <QProcess>

#include <algorithm>    // std::ranges::contains
#include <ranges>       // C++20 范围库

CommandHandler::CommandHandler(QObject* parent)
    : QObject{ parent } {
}

CommandHandler::~CommandHandler() {
}

CommandHandler::Result CommandHandler::executeCreate(const QString& path) {
    if (!validatePath(path)) {
        return std::unexpected(i18n("Invalid path: path %1 contains invalid characters", path));
    }

    const QFileInfo info(path);
    if (info.exists()) {
        return std::unexpected(i18n("Path %1 already exists", path));
    }

    StoreManager manager;
    manager.setStorePath(path);
    if (!manager.create()) {
        return std::unexpected(i18n("Failed to create store at %1", path));
    }

    return Result(i18n("Store created at %1", path));
}

CommandHandler::Result CommandHandler::executeLoad(const QString& storePath,
    const QString& mountPath) {
    // 检查挂载点是否已存在
    if (isPathMounted(mountPath)) {
        return std::unexpected(i18n("Path %1 already mounted", mountPath));
    }

    // 验证 store 路径
    const QFileInfo storeInfo(storePath);
    if (!storeInfo.exists() || !storeInfo.isDir()) {
        return std::unexpected(i18n("Invalid store path %1", storePath));
    }

    // 验证 store 是否有效
    StoreManager manager;
    manager.setStorePath(storePath);
    if (!manager.isValidStore()) {
        return std::unexpected(i18n("%1 is not a valid store", storePath));
    }

    // 创建挂载点目录
    QDir mountDir(mountPath);
    if (!mountDir.exists()) {
        if (!mountDir.mkpath(".")) {
            return std::unexpected(i18n("Failed to create mount directory at %1", mountPath));
        }
    }

    // 创建 FUSE 实例
    QSharedPointer<ClericumFuse> fuse(new ClericumFuse(this), &QObject::deleteLater);
    fuse->setStorePath(storePath);
    fuse->setMountPath(mountPath);

    // 连接信号
    QObject::connect(fuse.data(), &ClericumFuse::errorOccurred, this,
        [](const QString& error) {
            warn(i18n("FUSE error: %1", error));
        });

    // 直接挂载
    fuse->mount();

    return std::unexpected(i18n("Unexpected situation: fuse->mount() didn't daemonize this program"));
}

CommandHandler::Result CommandHandler::executeUnload(const QString& mountPath) {
    // 使用 fusermount 卸载
    QProcess proc;
    proc.start("fusermount3", { "-u", mountPath });
    return proc.waitForFinished()
        ? Result(QString())
        : std::unexpected(QString(i18n("Failed to unload %1", mountPath)));
}

CommandHandler::Result CommandHandler::executeBackup(const QString& virtualPath,
    const QString& backupName) {
    // 提取文件名
    const QString fileName = extractFileName(virtualPath);

    // 查找挂载点
    MountInfo mountInfo = findMountPoint(virtualPath);
    if (mountInfo.mountPath.isEmpty()) {
        return std::unexpected(i18n("Path %1 is not in mounted filesystem", virtualPath));
    }

    // 检查备份名是否有效
    if (backupName.isEmpty() || backupName.contains('/') ||
        backupName.contains(separator) || backupName.startsWith('.')) {
        return std::unexpected(i18n("Invalid backup name %1", backupName));
    }

    // 解析本源文件名
    const QString sourceName = extractSourceName(fileName);

    // 检查本源文件是否存在
    StoreManager storeManager;
    storeManager.setStorePath(mountInfo.storePath);
    if (!storeManager.sourceExists(sourceName)) {
        return std::unexpected(i18n("Source file %1 not found", sourceName));
    }

    // 检查备份名是否已存在，如果存在就先删除（C++23 std::ranges::contains）
    auto sourceInfo = storeManager.getSource(sourceName);
    if (std::ranges::contains(sourceInfo.backups, backupName, &BackupInfo::name)) {
        warn(i18n("Backup %1 already exists", backupName));
        QFile::remove(QStringList({ sourceInfo.backupsPath, backupName }).join("/"));
    }

    // 创建备份
    if (!storeManager.createBackup(sourceName, backupName)) {
        return std::unexpected(i18n("Failed to create backup"));
    }

    const QString msg = i18n("Backup %1 created for %2", backupName, sourceName);

    return Result(msg);
}

CommandHandler::Result CommandHandler::executeBackupLoad(const QString& backupFile) {
    // 提取文件名
    const QString fileName = extractFileName(backupFile);

    // 查找挂载点
    MountInfo mountInfo = findMountPoint(backupFile);
    if (mountInfo.mountPath.isEmpty()) {
        return std::unexpected(i18n("Path %1 is not in mounted filesystem", backupFile));
    }

    // 解析本源文件名
    const QString sourceName = extractSourceName(fileName);
    // 提取备份名
    const QString backupName = extractBackupNameFromVirtualPath(backupFile);

    // 检查本源文件是否存在
    StoreManager storeManager;
    storeManager.setStorePath(mountInfo.storePath);
    if (!storeManager.sourceExists(sourceName)) {
        return std::unexpected(i18n("Source file %1 not found", sourceName));
    }

    // 检查备份是否存在（C++23 std::ranges::contains）
    auto sourceInfo = storeManager.getSource(sourceName);
    const bool backupExists = std::ranges::contains(sourceInfo.backups, backupName, &BackupInfo::name);

    if (!backupExists) {
        return std::unexpected(i18n("Backup %1 not found", backupName));
    }

    // 从备份加载到 current
    if (!storeManager.loadBackup(sourceName, backupName)) {
        return std::unexpected(i18n("Failed to load backup %1", backupName));
    }

    const QString msg = i18n("Backup %1 loaded to %2", backupName, sourceName);

    return Result(msg);
}

CommandHandler::Result CommandHandler::executeBackupRemove(const QString& backupFile) {
    // 提取文件名
    const QString fileName = extractFileName(backupFile);

    // 查找挂载点
    MountInfo mountInfo = findMountPoint(backupFile);
    if (mountInfo.mountPath.isEmpty()) {
        return std::unexpected(i18n("Path %1 is not in mounted filesystem", backupFile));
    }

    // 解析本源文件名
    const QString sourceName = extractSourceName(fileName);
    // 提取备份名
    const QString backupName = extractBackupNameFromVirtualPath(backupFile);

    // 检查本源文件是否存在
    StoreManager storeManager;
    storeManager.setStorePath(mountInfo.storePath);
    if (!storeManager.sourceExists(sourceName)) {
        return std::unexpected(i18n("Source file %1 not found", sourceName));
    }

    // 检查备份是否存在（C++23 std::ranges::contains）
    auto sourceInfo = storeManager.getSource(sourceName);
    const bool backupExists = std::ranges::contains(sourceInfo.backups, backupName, &BackupInfo::name);

    if (!backupExists) {
        return std::unexpected(i18n("Backup %1 not found", backupName));
    }

    // 删除备份
    if (!storeManager.removeBackup(sourceName, backupName)) {
        return std::unexpected(i18n("Failed to remove backup %1", backupName));
    }

    const QString msg = i18n("Backup %1 removed from %2", backupName, sourceName);

    return Result(msg);
}

bool CommandHandler::isPathMounted(const QString& path) const {
    const MountInfo info = findMountPoint(path);
    return !info.mountPath.isEmpty();
}

CommandHandler::MountInfo CommandHandler::findMountPoint(const QString& path) const {
    // 检查路径中是否存在挂载点标记文件 (.clericum-mount，内容为 store 路径)
    QString checkPath = path;
    const QFileInfo info(checkPath);

    // 如果是文件而非目录，获取其所在目录
    if (info.isFile()) {
        checkPath = info.absolutePath();
    }

    MountInfo mountInfo;

    // 逐级向上查找标记文件
    QDir dir(checkPath);
    while (!dir.isRoot()) {
        if (dir.exists(MOUNT_MARKER_FILE)) {
            mountInfo.mountPath = dir.absolutePath();
            QFile file(dir.filePath(MOUNT_MARKER_FILE));
            if (!file.open(QIODevice::ReadOnly)) {
                warn(i18n("Failed to open mount marker file %1", file.fileName()));
                return mountInfo;
            }
            const QString content = file.readAll();
            file.close();
            mountInfo.storePath = content.trimmed();
        }
        if (!dir.cdUp()) {
            break;
        }
    }

    return mountInfo;
}

QString CommandHandler::extractFileName(const QString& virtualPath) {
    // 提取文件名（移除路径和前导 /）
    QString path = virtualPath;
    if (path.startsWith('/')) {
        path = path.mid(1);
    }

    int lastSlash = path.lastIndexOf('/');
    if (lastSlash >= 0) {
        path = path.mid(lastSlash + 1);
    }

    return path;
}

QString CommandHandler::extractSourceName(const QString& virtualPath) {
    const QString fileName = extractFileName(virtualPath);

    // 检查是否是备份文件（backupname-sourcename 格式）
    // 找到最后一个 - 后的部分
    int lastDash = fileName.lastIndexOf(separator);
    if (lastDash > 0) {
        // 检查 - 后的部分是否是已知的本源文件名
        const QString possibleSource = fileName.mid(lastDash + separator.length());

        // 如果去掉备份名前缀后，剩余部分可能是一个本源文件名
        // 这需要 StoreManager 来验证，但这里做简单检查
        if (!possibleSource.isEmpty()) {
            return possibleSource;
        }
    }

    return fileName;
}

QString CommandHandler::extractBackupNameFromVirtualPath(const QString& virtualPath) {
    const QString fileName = extractFileName(virtualPath);

    // 检查是否是备份文件（backupname-sourcename 格式）
    // 找到最后一个 - 前的部分
    int lastDash = fileName.lastIndexOf(separator);
    if (lastDash > 0) {
        // 返回 - 前的部分作为备份名
        return fileName.left(lastDash);
    }

    // 如果不是备份文件格式，返回空字符串
    return QString();
}

bool CommandHandler::validatePath(const QString& path) {
    if (path.isEmpty()) {
        return false;
    }

    // 检查路径中是否包含不安全的字符
    if (path.contains(QChar(0))) {
        return false;
    }

    // 检查是否包含明显的路径遍历
    if (path.contains("..")) {
        return false;
    }

    return true;
}
