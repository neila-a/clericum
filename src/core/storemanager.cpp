/**
 * @file storemanager.cpp
 * @brief StoreManager 类实现
 */

#include "storemanager.h"

#include <algorithm>    // std::ranges::find_if
#include <ranges>       // C++20 范围库

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

bool StoreManager::create() {
    QDir dir(m_storePath);

    // 检查路径是否已存在
    if (dir.exists()) {
        warn(i18n("Path %1 already exists", m_storePath));
        return false;
    }

    // 创建主目录
    if (!dir.mkpath(".")) {
        warn(i18n("Failed to create directory %1", m_storePath));
        return false;
    }

    // 创建标记文件（空文件）
    const QString markerFilePath = QStringList({ m_storePath, METADATA_FILENAME }).join("/");
    QFile markerFile(markerFilePath);
    if (!markerFile.open(QIODevice::WriteOnly)) {
        warn(i18n("Failed to create marker file %1", markerFilePath));
        return false;
    }
    markerFile.close();

    // 创建 files 和 backups 子目录
    const QString filesDir = QStringList({ m_storePath, FILES_DIRNAME }).join("/");
    const QString backupsDir = QStringList({ m_storePath, BACKUPS_DIRNAME }).join("/");
    if (!dir.mkpath(filesDir)) {
        warn(i18n("Failed to create files directory"));
        return false;
    }
    if (!dir.mkpath(backupsDir)) {
        warn(i18n("Failed to create backups directory"));
        return false;
    }

    // 设置路径并验证
    m_storePath = m_storePath;
    return isValidStore();
}

bool StoreManager::isValidStore() const {
    const QFileInfo info(m_storePath);
    if (!info.exists() || !info.isDir()) {
        warn(i18n("Not a dir"));
        return false;
    }

    const QString metaFilePath = QStringList({ m_storePath, METADATA_FILENAME }).join("/");
    const QFileInfo metaFile(metaFilePath);
    if (!metaFile.exists() || !metaFile.isFile()) {
        warn(i18n("No metafile"));
        return false;
    }

    // 检查 files 和 backups 子目录是否存在
    const QString filesDirPath = QStringList({ m_storePath, FILES_DIRNAME }).join("/");
    const QFileInfo filesDir(filesDirPath);
    const QString backupsDirPath = QStringList({ m_storePath, BACKUPS_DIRNAME }).join("/");
    const QFileInfo backupsDir(backupsDirPath);
    bool filesAndBackups = filesDir.exists() && filesDir.isDir() &&
        backupsDir.exists() && backupsDir.isDir();
    if (!filesAndBackups) {
        warn(i18n("No or invaild files or backups folder"));
    }
    return filesAndBackups;
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
            warn(i18n("Failed to create files directory %1", filesDirPath));
            return false;
        }
    }

    // 创建本源文件（空文件）
    QFile currentFile(filePath);
    if (!currentFile.open(QIODevice::WriteOnly)) {
        warn(i18n("Failed to create source file %1", filePath));
        return false;
    }
    currentFile.close();

    // 创建 backups/本源名 子目录
    if (!dir.mkpath(backupsDirPath)) {
        warn(i18n("Failed to create backups directory %1", backupsDirPath));
        return false;
    }

    return true;
}

bool StoreManager::createBackup(const QString& sourceName, const QString& backupName) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        warn(i18n("Source %1 not found", sourceName));
        return false;
    }
    QDir dir;
    dir.mkpath(sourceInfo.backupsPath);

    const QString backupPath = QStringList({ sourceInfo.backupsPath, backupName }).join("/");

    // 复制 current 文件到备份
    if (!QFile::copy(sourceInfo.currentPath, backupPath)) {
        warn(i18n("Failed to create backup at %1", backupPath));
        return false;
    }

    return true;
}

bool StoreManager::removeBackup(const QString& sourceName, const QString& backupName) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        warn(i18n("Source %1 not found", sourceName));
        return false;
    }

    // 查找并删除指定的备份文件（C++20 std::ranges::find_if 简化查找）
    const auto it = std::ranges::find_if(sourceInfo.backups,
        [&backupName](const BackupInfo& b) { return b.name == backupName; });
    if (it != sourceInfo.backups.end()) {
        if (QFile::remove(it->fullPath)) {
            sourceInfo.backups.erase(it);   // 从备份列表中移除
            return true;
        }
        warn(i18n("Failed to remove backup file %1", it->fullPath));
        return false;
    }

    warn(i18n("Backup %1 not found", backupName));
    return false;
}

bool StoreManager::loadBackup(const QString& sourceName, const QString& backupName) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        warn(i18n("Source %1 not found", sourceName));
        return false;
    }

    // 查找指定的备份（C++20 std::ranges::find_if）
    const auto backup = std::ranges::find_if(sourceInfo.backups,
        [&backupName](const BackupInfo& b) { return b.name == backupName; });
    if (backup == sourceInfo.backups.end()) {
        warn(i18n("Backup %1 not found", backupName));
        return false;
    }

    // 复制备份文件到 current
    QFile::remove(sourceInfo.currentPath); // 必须显式覆盖
    if (!QFile::copy(backup->fullPath, sourceInfo.currentPath)) {
        warn(i18n("Failed to load backup %1 to current %2", backup->fullPath, sourceInfo.currentPath));
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
            const QString virtualName = backup.name + separator + sourceInfo.name;
            result.insert(virtualName, backup.fullPath);
        }
    }

    return result;
}

bool StoreManager::parseVirtualName(const QString& virtualName,
    QString& actualSourceName,
    bool& isBackup) const {
    // 检查是否是备份文件（格式：备份名-本源名）
    // 需要找到最后一个 separator 后的本源名

    const QStringList sourceNames = scanSources().keys();
    QString longestMatch;

    for (const QString& sourceName : sourceNames) {
        const QString prefix = QStringLiteral(" - %1").arg(sourceName);
        if (virtualName.endsWith(prefix)) {
            // 检查这个前缀是否是有效的（即前面的部分是备份名）
            const QString backupName = virtualName.left(virtualName.length() - prefix.length());
            if (!backupName.isEmpty() && !backupName.contains(separator)) {
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
