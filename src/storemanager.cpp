/**
 * @file storemanager.cpp
 * @brief StoreManager 类实现
 */

#include "storemanager.h"

StoreManager::StoreManager(QObject* parent)
    : QObject{ parent } {
}

StoreManager::~StoreManager() = default;

void StoreManager::setStorePath(const QString& path) {
    m_storePath = path;
}

QString StoreManager::storePath() const {
    return m_storePath;
}

bool StoreManager::create(const QString& path) {
    QDir dir(path);

    // 检查路径是否已存在
    if (dir.exists()) {
        warn("Path %1 already exists", path);
        return false;
    }

    // 创建主目录
    if (!dir.mkpath(".")) {
        warn("Failed to create directory %1", path);
        return false;
    }

    // 创建标记文件（空文件）
    const QString markerFilePath = QStringList({ m_storePath, METADATA_FILENAME }).join("/");
    QFile markerFile(markerFilePath);
    if (!markerFile.open(QIODevice::WriteOnly)) {
        warn("Failed to create marker file", "");
        return false;
    }
    markerFile.close();

    // 创建 files 和 backups 子目录
    const QString filesDir = QStringList({ m_storePath, FILES_DIRNAME }).join("/");
    const QString backupsDir = QStringList({ m_storePath, BACKUPS_DIRNAME }).join("/");
    if (!dir.mkpath(filesDir)) {
        warn("Failed to create files directory", "");
        return false;
    }
    if (!dir.mkpath(backupsDir)) {
        warn("Failed to create backups directory", "");
        return false;
    }

    // 设置路径并验证
    m_storePath = path;
    return isValidStore();
}

bool StoreManager::isValidStore(const QString& path) const {
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        warn("Not a dir", "");
        return false;
    }

    const QString metaFilePath = QStringList({ path, METADATA_FILENAME }).join("/");
    const QFileInfo metaFile(metaFilePath);
    if (!metaFile.exists() || !metaFile.isFile()) {
        warn("No metafile", "");
        return false;
    }

    // 检查 files 和 backups 子目录是否存在
    const QString filesDirPath = QStringList({ path, FILES_DIRNAME }).join("/");
    const QFileInfo filesDir(filesDirPath);
    const QString backupsDirPath = QStringList({ path, BACKUPS_DIRNAME }).join("/");
    const QFileInfo backupsDir(backupsDirPath);
    bool filesAndBackups = filesDir.exists() && filesDir.isDir() &&
        backupsDir.exists() && backupsDir.isDir();
    if (!filesAndBackups) {
        warn("No or invaild files or backups folder", "");
    }
    return filesAndBackups;
}

bool StoreManager::isValidStore() const {
    return isValidStore(m_storePath);
}

QMap<QString, SourceInfo> StoreManager::scanSources() const {
    QMap<QString, SourceInfo> result;

    if (!isValidStore()) {
        return result;
    }

    const QString filesDirPath = QStringList({ m_storePath, FILES_DIRNAME }).join("/");
    QDir filesDir(filesDirPath);
    if (!filesDir.exists()) {
        return result;
    }

    const auto entries = filesDir.entryList(QDir::Files);

    for (const QString& entry : entries) {
        // 跳过元数据文件
        if (entry == METADATA_FILENAME) {
            continue;
        }

        SourceInfo sourceInfo;
        sourceInfo.name = entry;
        sourceInfo.currentPath = QStringList({ m_storePath, FILES_DIRNAME, entry }).join("/");
        sourceInfo.backupsPath = QStringList({ m_storePath, BACKUPS_DIRNAME, entry }).join("/");

        // 扫描备份文件
        const QDir backupsDir(sourceInfo.backupsPath);
        if (backupsDir.exists()) {
            const auto backupEntries = backupsDir.entryList(QDir::Files);
            for (const QString& backupEntry : backupEntries) {
                BackupInfo backupInfo;
                backupInfo.name = backupEntry;
                backupInfo.fullPath = QStringList({ sourceInfo.backupsPath, backupEntry }).join("/");
                sourceInfo.backups.append(backupInfo);
            }
        }

        result.insert(entry, sourceInfo);
    }

    return result;
}

SourceInfo StoreManager::getSource(const QString& name) const {
    return scanSources().value(name);
}

QString StoreManager::getCurrentPath(const QString& name) const {
    SourceInfo info = getSource(name);
    return info.currentPath;
}

bool StoreManager::createSource(const QString& name) {
    const QString filesDirPath = QStringList({ m_storePath, FILES_DIRNAME }).join("/");
    const QString filePath = QStringList({ filesDirPath, name }).join("/");
    const QString backupsDirPath = QStringList({ m_storePath, BACKUPS_DIRNAME, name }).join("/");

    // 确保 files 目录存在
    QDir dir;
    if (!dir.exists(filesDirPath)) {
        if (!dir.mkpath(filesDirPath)) {
            warn("Failed to create files directory %1", filesDirPath);
            return false;
        }
    }

    // 创建本源文件（空文件）
    QFile currentFile(filePath);
    if (!currentFile.open(QIODevice::WriteOnly)) {
        warn("Failed to create source file %1", filePath);
        return false;
    }
    currentFile.close();

    // 创建 backups/本源名 子目录
    if (!dir.mkpath(backupsDirPath)) {
        warn("Failed to create backups directory %1", backupsDirPath);
        return false;
    }

    return true;
}

bool StoreManager::createBackup(const QString& sourceName, const QString& backupName) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        warn("Source %1 not found", sourceName);
        return false;
    }
    QDir dir;
    dir.mkpath(sourceInfo.backupsPath);

    const QString backupPath = QStringList({ sourceInfo.backupsPath, backupName }).join("/");

    // 复制 current 文件到备份
    if (!QFile::copy(sourceInfo.currentPath, backupPath)) {
        warn("Failed to create backup at %1", backupPath);
        return false;
    }

    return true;
}

bool StoreManager::loadBackup(const QString& sourceName, const QString& backupName) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        warn("Source %1 not found", sourceName);
        return false;
    }

    // 查找指定的备份
    QString backupPath;
    for (const BackupInfo& backup : sourceInfo.backups) {
        if (backup.name == backupName) {
            backupPath = backup.fullPath;
            break;
        }
    }

    if (backupPath.isEmpty()) {
        warn("Backup %1 not found", backupName);
        return false;
    }

    // 复制备份文件到 current
    QFile::remove(sourceInfo.currentPath); // 必须显式覆盖
    if (!QFile::copy(backupPath, sourceInfo.currentPath)) {
        warn("Failed to load backup %1 to current %2", backupPath, sourceInfo.currentPath);
        return false;
    }

    return true;
}

bool StoreManager::sourceExists(const QString& name) const {
    return scanSources().contains(name);
}

QMap<QString, QString> StoreManager::getFlatFileList() const {
    QMap<QString, QString> result;

    for (const SourceInfo& sourceInfo : scanSources().values()) {
        // 添加本源文件
        if (QFileInfo::exists(sourceInfo.currentPath)) {
            result.insert(sourceInfo.name, sourceInfo.currentPath);
        }

        // 添加备份文件（命名为 "备份名 - 本源名"）
        for (const BackupInfo& backup : sourceInfo.backups) {
            const QString virtualName = QStringLiteral("%1 - %2").arg(backup.name, sourceInfo.name);
            result.insert(virtualName, backup.fullPath);
        }
    }

    return result;
}

bool StoreManager::parseVirtualName(const QString& virtualName,
    QString& actualSourceName,
    bool& isBackup) const {
    // 检查是否是备份文件（格式：备份名-本源名）
    // 需要找到最后一个 '-' 后的本源名

    const QStringList sourceNames = scanSources().keys();
    QString longestMatch;

    for (const QString& sourceName : sourceNames) {
        const QString prefix = QStringLiteral(" - %1").arg(sourceName);
        if (virtualName.endsWith(prefix)) {
            // 检查这个前缀是否是有效的（即前面的部分是备份名）
            const QString backupName = virtualName.left(virtualName.length() - prefix.length());
            if (!backupName.isEmpty() && !backupName.contains(" - ")) {
                // 找到一个匹配
                if (prefix.length() > longestMatch.length()) {
                    longestMatch = prefix;
                    actualSourceName = sourceName;
                    isBackup = true;
                }
            }
        }
    }

    if (!longestMatch.isEmpty()) {
        return true;
    }

    // 检查是否是本源文件名
    if (sourceNames.contains(virtualName)) {
        actualSourceName = virtualName;
        isBackup = false;
        return true;
    }

    return false;
}

QString StoreManager::resolveRealPath(const QString& virtualName) const {
    QString sourceName;
    bool isBackup;

    if (parseVirtualName(virtualName, sourceName, isBackup)) {
        SourceInfo info = getSource(sourceName);
        return info.currentPath;
    }

    return QString();
}

bool StoreManager::isBackupFile(const QString& virtualName) const {
    QString sourceName;
    bool isBackup;

    if (parseVirtualName(virtualName, sourceName, isBackup)) {
        return isBackup;
    }

    return false;
}
