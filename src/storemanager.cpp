/**
 * @file storemanager.cpp
 * @brief StoreManager 类实现
 */

#include "storemanager.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>

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
        qWarning() << "Path already exists:" << path;
        return false;
    }

    // 创建主目录
    if (!dir.mkpath(".")) {
        qWarning() << "Failed to create directory:" << path;
        return false;
    }

    // 创建标记文件（空文件）
    QFile markerFile(path + "/" + METADATA_FILENAME);
    if (!markerFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create marker file";
        return false;
    }
    markerFile.close();

    // 创建 files 和 backups 子目录
    if (!dir.mkpath(path + "/" + FILES_DIRNAME)) {
        qWarning() << "Failed to create files directory";
        return false;
    }
    if (!dir.mkpath(path + "/" + BACKUPS_DIRNAME)) {
        qWarning() << "Failed to create backups directory";
        return false;
    }

    // 设置路径并验证
    m_storePath = path;
    return isValidStore();
}

bool StoreManager::isValidStore(const QString& path) const {
    QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        return false;
    }

    QFileInfo metaFile(path + "/" + METADATA_FILENAME);
    if (!metaFile.exists() || !metaFile.isFile()) {
        return false;
    }

    // 检查 files 和 backups 子目录是否存在
    QFileInfo filesDir(path + "/" + FILES_DIRNAME);
    QFileInfo backupsDir(path + "/" + BACKUPS_DIRNAME);
    return filesDir.exists() && filesDir.isDir() &&
           backupsDir.exists() && backupsDir.isDir();
}

bool StoreManager::isValidStore() const {
    return isValidStore(m_storePath);
}

QMap<QString, SourceInfo> StoreManager::scanSources() const {
    QMap<QString, SourceInfo> result;

    if (!isValidStore()) {
        return result;
    }

    QDir filesDir(m_storePath + "/" + FILES_DIRNAME);
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
        sourceInfo.currentPath = m_storePath + "/" + FILES_DIRNAME + "/" + entry;
        sourceInfo.backupsPath = m_storePath + "/" + BACKUPS_DIRNAME + "/" + entry;

        // 扫描备份文件
        QDir backupsDir(sourceInfo.backupsPath);
        if (backupsDir.exists()) {
            const auto backupEntries = backupsDir.entryList(QDir::Files);
            for (const QString& backupEntry : backupEntries) {
                BackupInfo backupInfo;
                backupInfo.name = backupEntry;
                backupInfo.fullPath = sourceInfo.backupsPath + "/" + backupEntry;
                sourceInfo.backups.append(backupInfo);
            }
        }

        result.insert(entry, sourceInfo);
    }

    return result;
}

QVector<SourceInfo> StoreManager::getAllSources() const {
    return QVector<SourceInfo>::fromList(scanSources().values());
}

SourceInfo StoreManager::getSource(const QString& name) const {
    return scanSources().value(name);
}

QString StoreManager::getCurrentPath(const QString& name) const {
    SourceInfo info = getSource(name);
    return info.currentPath;
}

bool StoreManager::createSource(const QString& name) {
    QString filesDirPath = m_storePath + "/" + FILES_DIRNAME;
    QString filePath = filesDirPath + "/" + name;
    QString backupsDirPath = m_storePath + "/" + BACKUPS_DIRNAME + "/" + name;

    // 确保 files 目录存在
    QDir dir;
    if (!dir.exists(filesDirPath)) {
        if (!dir.mkpath(filesDirPath)) {
            qWarning() << "Failed to create files directory:" << filesDirPath;
            return false;
        }
    }

    // 创建本源文件（空文件）
    QFile currentFile(filePath);
    if (!currentFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create source file:" << filePath;
        return false;
    }
    currentFile.close();

    // 创建 backups/本源名 子目录
    if (!dir.mkpath(backupsDirPath)) {
        qWarning() << "Failed to create backups directory:" << backupsDirPath;
        return false;
    }

    return true;
}

bool StoreManager::createBackup(const QString& sourceName, const QString& backupName) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        qWarning() << "Source not found:" << sourceName;
        return false;
    }
    QDir dir;
    dir.mkpath(sourceInfo.backupsPath);

    QString backupPath = sourceInfo.backupsPath + "/" + backupName;

    // 复制 current 文件到备份
    if (!QFile::copy(sourceInfo.currentPath, backupPath)) {
        qWarning() << "Failed to create backup:" << backupPath;
        return false;
    }

    return true;
}

bool StoreManager::loadBackup(const QString& sourceName, const QString& backupName) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        qWarning() << "Source not found:" << sourceName;
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
        qWarning() << "Backup not found:" << backupName;
        return false;
    }

    // 复制备份文件到 current
    QFile::remove(sourceInfo.currentPath); // 必须显式覆盖
    if (!QFile::copy(backupPath, sourceInfo.currentPath)) {
        qWarning() << "Failed to load backup " << backupPath << " to current " << sourceInfo.currentPath;
        return false;
    }

    return true;
}

bool StoreManager::copyToCurrent(const QString& sourceName, const QString& sourceFile) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        qWarning() << "Source not found:" << sourceName;
        return false;
    }

    // 如果本源文件条目不存在，先创建
    if (!QFileInfo::exists(sourceInfo.currentPath)) {
        if (!createSource(sourceName)) {
            return false;
        }
    }

    // 复制文件到本源文件
    if (!QFile::copy(sourceFile, sourceInfo.currentPath)) {
        qWarning() << "Failed to copy " << sourceFile << " to " << sourceInfo.currentPath;
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
            QString virtualName = backup.name + " - " + sourceInfo.name;
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
        QString prefix = " - " + sourceName;
        if (virtualName.endsWith(prefix)) {
            // 检查这个前缀是否是有效的（即前面的部分是备份名）
            QString backupName = virtualName.left(virtualName.length() - prefix.length());
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

QStringList StoreManager::getSourceNames() const {
    return scanSources().keys();
}
