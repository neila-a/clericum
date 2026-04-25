#include "clericumfuse.h"

int ClericumFuse::fuseGetattr(const char* path, struct stat* stbuf,
    fuse_file_info* fi) {
    Q_UNUSED(fi);

    if (!s_instance) {
        return -ENOENT;
    }

    memset(stbuf, 0, sizeof(struct stat));

    QString pathStr = QString::fromUtf8(path);

    // 根目录
    if (pathStr == "/") {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    // 移除前导 /
    pathStr = pathStr.mid(1);

    // 检查是否是挂载点标记文件
    if (pathStr == MOUNT_MARKER_FILE) {
        const QByteArray storePathData = s_instance->storePath().toUtf8();
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = storePathData.size();
        stbuf->st_mtime = QDateTime::currentSecsSinceEpoch();
        stbuf->st_atime = stbuf->st_mtime;
        stbuf->st_ctime = stbuf->st_mtime;
        return 0;
    }

    const QString realPath = s_instance->getFileList().value(pathStr);

    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    const QFileInfo fileInfo(realPath);
    if (!fileInfo.exists()) {
        return -ENOENT;
    }

    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;
    stbuf->st_size = static_cast<off_t>(fileInfo.size());
    stbuf->st_mtime = fileInfo.lastModified().toSecsSinceEpoch();
    stbuf->st_atime = fileInfo.lastRead().toSecsSinceEpoch();
    stbuf->st_ctime = fileInfo.lastModified().toSecsSinceEpoch();

    return 0;
}

int ClericumFuse::fuseUnlink(const char* path) {
    if (!s_instance) {
        return -ENOENT;
    }

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    // 不允许删除挂载点标记文件
    if (pathStr == MOUNT_MARKER_FILE) {
        return -EACCES;
    }

    // 检查文件是否存在
    const QString realPath = s_instance->getFileList().value(pathStr);
    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    // 如果是备份文件，不能删除备份文件
    if (s_instance->m_storeManager->isBackupFile(pathStr)) {
        return -EACCES;
    }

    // 如果是本源文件，删除本源文件
    const QString sourceName = pathStr;
    SourceInfo sourceInfo = s_instance->m_storeManager->getSource(sourceName);
    if (sourceInfo.name.isEmpty()) {
        return -ENOENT;
    }

    // 删除本源文件
    QFile sourceFile(sourceInfo.currentPath);
    if (!sourceFile.remove()) {
        return -EIO;
    }

    // 更新文件列表
    s_instance->getFileList() = s_instance->m_storeManager->getFlatFileList();
    return 0;
}

int ClericumFuse::fuseAccess(const char* path, int mask) {
    if (!s_instance) {
        return -ENOENT;
    }

    QString pathStr = QString::fromUtf8(path);

    // 根目录总是可访问
    if (pathStr == "/") {
        return 0;
    }

    // 移除前导 /
    pathStr = pathStr.mid(1);

    // 检查是否是挂载点标记文件
    if (pathStr == MOUNT_MARKER_FILE) {
        return 0;  // 标记文件只读
    }

    const QString realPath = s_instance->getFileList().value(pathStr);

    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    const QFileInfo fileInfo(realPath);
    if (!fileInfo.exists()) {
        return -ENOENT;
    }

    return 0;
}

int ClericumFuse::fuseReaddir(const char* path, void* buf,
    fuse_fill_dir_t filler,
    off_t offset,
    fuse_file_info* fi,
    fuse_readdir_flags flags) {
    Q_UNUSED(offset);
    Q_UNUSED(fi);
    Q_UNUSED(flags);

    if (!s_instance) {
        return -ENOENT;
    }

    const QString pathStr = QString::fromUtf8(path);

    // 只处理根目录
    if (pathStr != "/") {
        return -ENOENT;
    }

    // 添加 . 和 ..
    filler(buf, ".", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));
    filler(buf, "..", nullptr, 0, static_cast<fuse_fill_dir_flags>(0));

    // 添加挂载点标记文件
    filler(buf, MOUNT_MARKER_FILE, nullptr, 0, static_cast<fuse_fill_dir_flags>(0));

    // 添加所有虚拟文件
    for (const QString& virtualPath : s_instance->getFileList().keys()) {
        filler(buf, qPrintable(virtualPath), nullptr, 0,
            static_cast<fuse_fill_dir_flags>(0));
    }

    return 0;
}

int ClericumFuse::fuseOpen(const char* path, fuse_file_info* fi) {
    if (!s_instance) {
        return -ENOENT;
    }

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    // 检查是否是挂载点标记文件
    if (pathStr == MOUNT_MARKER_FILE) {
        if ((fi->flags & O_ACCMODE) != O_RDONLY) {
            return -EACCES;  // 标记文件只读
        }
        return 0;
    }

    const QString realPath = s_instance->getFileList().value(pathStr);

    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    return 0;
}

int ClericumFuse::fuseRead(const char* path, char* buf, size_t size,
    off_t offset, fuse_file_info* fi) {
    Q_UNUSED(fi);

    if (!s_instance) {
        return -ENOENT;
    }

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    // 检查是否是挂载点标记文件
    if (pathStr == MOUNT_MARKER_FILE) {
        const QByteArray storePathData = s_instance->storePath().toUtf8();
        if (offset < storePathData.size()) {
            int avail = storePathData.size() - static_cast<int>(offset);
            if (static_cast<int>(size) < avail) {
                avail = static_cast<int>(size);
            }
            memcpy(buf, storePathData.constData() + offset, avail);
            return avail;
        }
        return 0;
    }

    const QString realPath = s_instance->getFileList().value(pathStr);

    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    QFile file(realPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return -EIO;
    }

    if (!file.seek(offset)) {
        return -EIO;
    }

    qint64 bytesRead = file.read(buf, size);
    file.close();

    return static_cast<int>(bytesRead);
}

int ClericumFuse::fuseWrite(const char* path, const char* buf, size_t size,
    off_t offset, fuse_file_info* fi) {
    Q_UNUSED(fi);

    if (!s_instance) {
        return -ENOENT;
    }

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    QString realPath;
    // 备份文件的写入重定向到本源文件
    if (s_instance->m_storeManager->isBackupFile(pathStr)) {
        // 使用 resolveRealPath 获取本源文件的 current 路径进行写入
        realPath = s_instance->m_storeManager->resolveRealPath(pathStr);

        // 用备份文件覆盖本源文件，以符合offset
        if (!QFile::remove(realPath)) {
            return -EIO;
        }
        if (!QFile::copy(s_instance->getFileList().value(pathStr), realPath)) {
            return -EIO;
        }
    } else {
        realPath = s_instance->getFileList().value(pathStr);
    }

    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    QFile file(realPath);
    if (!file.open(QIODevice::ReadWrite)) {
        return -EIO;
    }

    if (!file.seek(offset)) {
        file.close();
        return -EIO;
    }

    qint64 bytesWritten = file.write(buf, size);
    file.close();

    return static_cast<int>(bytesWritten);
}

int ClericumFuse::fuseCreate(const char* path, mode_t mode,
    fuse_file_info* fi) {
    Q_UNUSED(mode);

    if (!s_instance) {
        return -ENOENT;
    }

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    // 不允许创建备份文件
    if (s_instance->m_storeManager->isBackupFile(pathStr)) {
        return -EACCES;
    }

    // 创建本源文件
    const QString sourceName = pathStr;

    // 确保本源文件条目存在
    if (!s_instance->m_storeManager->sourceExists(sourceName)) {
        // 确保 files 目录存在
        QDir dir;
        const QString filesDir = QStringList({ s_instance->storePath(), StoreManager::FILES_DIRNAME }).join("/");
        if (!dir.exists(filesDir)) {
            dir.mkpath(filesDir);
        }
        s_instance->m_storeManager->createSource(sourceName);
    }

    const QString realPath = s_instance->m_storeManager->getCurrentPath(sourceName);

    // 创建空文件
    QFile file(realPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return -EIO;
    }
    file.close();

    // 更新文件列表
    s_instance->getFileList() = s_instance->m_storeManager->getFlatFileList();

    fi->flags |= O_WRONLY;
    return 0;
}

int ClericumFuse::fuseRename(const char* from, const char* to, unsigned int flags) {
    Q_UNUSED(flags);
    if (!s_instance) {
        return -ENOENT;
    }

    const QString fromPathStr = QString::fromUtf8(from).mid(1);  // 移除前导 /
    const QString toPathStr = QString::fromUtf8(to).mid(1);     // 移除前导 /

    // 不允许重命名挂载点标记文件
    if (fromPathStr == MOUNT_MARKER_FILE || toPathStr == MOUNT_MARKER_FILE) {
        return -EACCES;
    }

    // 检查源文件是否存在
    const QString realFromPath = s_instance->getFileList().value(fromPathStr);
    if (realFromPath.isEmpty()) {
        return -ENOENT;
    }

    // 判断是本源文件还是备份文件
    if (s_instance->m_storeManager->isBackupFile(fromPathStr)) {
        // 重命名备份文件
        QString fromSourceName, toSourceName;
        bool isBackupFrom, isBackupTo;

        // 解析源文件名
        if (!s_instance->m_storeManager->parseVirtualName(fromPathStr, fromSourceName, isBackupFrom)) {
            return -ENOENT;
        }

        // 解析目标文件名
        if (!s_instance->m_storeManager->parseVirtualName(toPathStr, toSourceName, isBackupTo)) {
            // 目标可能是一个新的本源文件名
            // 不允许将备份文件重命名为本源文件名
            return -EACCES;
        }

        // 获取备份信息
        SourceInfo fromSourceInfo = s_instance->m_storeManager->getSource(fromSourceName);
        SourceInfo toSourceInfo = s_instance->m_storeManager->getSource(toSourceName);
        if (fromSourceInfo.name.isEmpty() || toSourceInfo.name.isEmpty()) {
            return -ENOENT;
        }

        // 从虚拟名中提取备份名
        // fromPathStr 格式为 "备份名 - 本源名"
        const QString fromBackupName = fromPathStr.left(fromPathStr.indexOf(" - "));
        const QString fromBackupPath = QStringList({ fromSourceInfo.backupsPath, fromBackupName }).join("/");

        QFile backupFile(fromBackupPath);
        if (fromSourceName == toSourceName) {
            // 同一个本源文件，只需要重命名备份文件
            const QString toBackupName = toPathStr.left(toPathStr.indexOf(" - "));
            const QString toBackupPath = QStringList({ toSourceInfo.backupsPath, toBackupName }).join("/");

            // 重命名备份文件
            QFile::remove(toBackupPath);
            if (!backupFile.rename(toBackupPath)) {
                return -EIO;
            }
        } else {
            // 不同的本源文件，覆盖到其本源文件
            QFile::remove(toSourceInfo.currentPath);
            if (!backupFile.rename(toSourceInfo.currentPath)) {
                return -EIO;
            }
        }
    } else {
        // 重命名本源文件
        // 获取本源信息
        SourceInfo sourceInfo = s_instance->m_storeManager->getSource(fromPathStr);
        if (sourceInfo.name.isEmpty()) {
            return -ENOENT;
        }
        QFile sourceFile(sourceInfo.currentPath);

        QString dummySourceName;
        bool dummyIsBackup;
        if (s_instance->m_storeManager->parseVirtualName(toPathStr, dummySourceName, dummyIsBackup) && dummyIsBackup) {
            // 重命名为备份文件名格式则重定向到覆盖其本源文件
            SourceInfo toSourceInfo = s_instance->m_storeManager->getSource(dummySourceName);
            QFile::remove(toSourceInfo.currentPath);
            if (!sourceFile.rename(toSourceInfo.currentPath)) {
                return -EIO;
            }
        } else {
            // 重命名本源文件（files/下的文件）
            const QString newCurrentPath = QStringList({ s_instance->storePath(), StoreManager::FILES_DIRNAME, toPathStr }).join("/");
            if (!sourceFile.rename(newCurrentPath)) {
                return -EIO;
            }
        }
    }

    // 更新文件列表
    s_instance->getFileList() = s_instance->m_storeManager->getFlatFileList();
    return 0;
}
