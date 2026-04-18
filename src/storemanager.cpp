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
    m_cacheValid = false;
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
    return metaFile.exists() && metaFile.isFile();
}

bool StoreManager::isValidStore() const {
    return isValidStore(m_storePath);
}

void StoreManager::refreshCache() const {
    m_cache.clear();

    if (!isValidStore()) {
        m_cacheValid = false;
        return;
    }

    QDir storeDir(m_storePath);
    const auto entries = storeDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& entry : entries) {
        // 跳过元数据文件
        if (entry == METADATA_FILENAME) {
            continue;
        }

        SourceInfo sourceInfo;
        sourceInfo.name = entry;
        sourceInfo.fullPath = m_storePath + "/" + entry;
        sourceInfo.currentPath = sourceInfo.fullPath + "/" + CURRENT_FILENAME;
        sourceInfo.backupsPath = sourceInfo.fullPath + "/" + BACKUPS_DIRNAME;

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

        m_cache.insert(entry, sourceInfo);
    }

    m_cacheValid = true;
}

QVector<SourceInfo> StoreManager::getAllSources() const {
    if (!m_cacheValid) {
        refreshCache();
    }

    return QVector<SourceInfo>::fromList(m_cache.values());
}

SourceInfo StoreManager::getSource(const QString& name) const {
    if (!m_cacheValid) {
        refreshCache();
    }

    return m_cache.value(name);
}

QString StoreManager::getCurrentPath(const QString& name) const {
    SourceInfo info = getSource(name);
    return info.currentPath;
}

bool StoreManager::createSource(const QString& name) {
    QString sourcePath = m_storePath + "/" + name;

    QDir dir;
    if (!dir.mkpath(sourcePath)) {
        qWarning() << "Failed to create source directory:" << sourcePath;
        return false;
    }

    // 创建 current 文件（空文件）
    QFile currentFile(sourcePath + "/" + CURRENT_FILENAME);
    if (!currentFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to create current file";
        return false;
    }
    currentFile.close();

    // 创建 backups 文件夹
    if (!dir.mkpath(sourcePath + "/" + BACKUPS_DIRNAME)) {
        qWarning() << "Failed to create backups directory";
        return false;
    }

    m_cacheValid = false;
    return true;
}

bool StoreManager::createBackup(const QString& sourceName, const QString& backupName) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        qWarning() << "Source not found:" << sourceName;
        return false;
    }

    QString backupPath = sourceInfo.backupsPath + "/" + backupName;

    // 复制 current 文件到备份
    if (!QFile::copy(sourceInfo.currentPath, backupPath)) {
        qWarning() << "Failed to create backup:" << backupPath;
        return false;
    }

    m_cacheValid = false;
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

    m_cacheValid = false;
    return true;
}

bool StoreManager::copyToCurrent(const QString& sourceName, const QString& sourceFile) {
    SourceInfo sourceInfo = getSource(sourceName);
    if (sourceInfo.currentPath.isEmpty()) {
        qWarning() << "Source not found:" << sourceName;
        return false;
    }

    // 如果本源文件条目不存在，先创建
    if (!QFileInfo::exists(sourceInfo.fullPath)) {
        if (!createSource(sourceName)) {
            return false;
        }
    }

    // 复制文件到 current
    if (!QFile::copy(sourceFile, sourceInfo.currentPath)) {
        qWarning() << "Failed to copy " << sourceFile << " to " << sourceInfo.currentPath;
        return false;
    }

    m_cacheValid = false;
    return true;
}

bool StoreManager::sourceExists(const QString& name) const {
    if (!m_cacheValid) {
        refreshCache();
    }

    return m_cache.contains(name);
}

QMap<QString, QString> StoreManager::getFlatFileList() const {
    QMap<QString, QString> result;

    if (!m_cacheValid) {
        refreshCache();
    }

    for (const SourceInfo& sourceInfo : m_cache.values()) {
        // 添加本源文件（current）
        if (QFileInfo::exists(sourceInfo.currentPath)) {
            result.insert(sourceInfo.name, sourceInfo.currentPath);
        }

        // 添加备份文件（命名为 "备份名-本源名"）
        for (const BackupInfo& backup : sourceInfo.backups) {
            QString virtualName = backup.name + " - " + sourceInfo.name;
            result.insert(virtualName, sourceInfo.backupsPath + "/" + backup.name);
        }
    }

    return result;
}

bool StoreManager::parseVirtualName(const QString& virtualName,
    QString& actualSourceName,
    bool& isBackup) const {
    // 检查是否是备份文件（格式：备份名-本源名）
    // 需要找到最后一个 '-' 后的本源名

    if (!m_cacheValid) {
        refreshCache();
    }

    const QStringList sourceNames = m_cache.keys();
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
    if (!m_cacheValid) {
        refreshCache();
    }

    return m_cache.keys();
}
