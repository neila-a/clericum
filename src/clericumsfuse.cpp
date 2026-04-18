/**
 * @file clericumsfuse.cpp
 * @brief ClericumFuse 类实现
 */

#include "clericumsfuse.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QProcess>
#include <cerrno>
#include <cstring>
#include <memory>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

 // FUSE 3 API
#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>

// 静态成员初始化
ClericumFuse* ClericumFuse::s_instance = nullptr;

ClericumFuse::ClericumFuse(QObject* parent)
    : QObject{ parent }
    , m_storeManager(new StoreManager(this)) {
    s_instance = this;
}

ClericumFuse::~ClericumFuse() {
    s_instance = nullptr;
}

void ClericumFuse::setStorePath(const QString& path) {
    m_storeManager->setStorePath(path);
}

QString ClericumFuse::storePath() const {
    return m_storeManager->storePath();
}

void ClericumFuse::setMountPath(const QString& path) {
    m_mountPath = path;
}

QString ClericumFuse::mountPath() const {
    return m_mountPath;
}

ClericumFuse::MountStatus ClericumFuse::status() const {
    return m_status;
}

void ClericumFuse::refreshCache() {
    m_storeManager->refreshCache();
    m_fileList = m_storeManager->getFlatFileList();
}

QStringList ClericumFuse::fileNames() const {
    return m_fileList.keys();
}

QString ClericumFuse::resolvePath(const QString& name) const {
    return m_fileList.value(name, QString());
}

bool ClericumFuse::isBackupFile(const QString& name) const {
    return m_storeManager->isBackupFile(name);
}

QSharedPointer<StoreManager> ClericumFuse::storeManager() const {
    return m_storeManager;
}

// ============== FUSE 回调实现 ==============

int ClericumFuse::fuseGetattr(const char* path, struct stat* stbuf,
    struct fuse_file_info* fi) {
    Q_UNUSED(fi);

    if (!s_instance) {
        return -ENOENT;
    }
    s_instance->refreshCache();

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
        QByteArray storePathData = s_instance->storePath().toUtf8();
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = storePathData.size();
        stbuf->st_mtime = QDateTime::currentSecsSinceEpoch();
        stbuf->st_atime = stbuf->st_mtime;
        stbuf->st_ctime = stbuf->st_mtime;
        return 0;
    }

    QString realPath = s_instance->m_fileList.value(pathStr);

    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    QFileInfo fileInfo(realPath);
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
    s_instance->refreshCache();

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    // 不允许删除挂载点标记文件
    if (pathStr == MOUNT_MARKER_FILE) {
        return -EACCES;
    }

    // 检查文件是否存在
    QString realPath = s_instance->m_fileList.value(pathStr);
    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    // 如果是备份文件，删除备份文件
    if (s_instance->m_storeManager->isBackupFile(pathStr)) {
        QFile file(realPath);
        if (!file.remove()) {
            return -EIO;
        }
        // 更新文件列表
        s_instance->m_fileList = s_instance->m_storeManager->getFlatFileList();
        return 0;
    }

    // 如果是本源文件，删除整个本源文件夹
    QString sourceName = pathStr;
    SourceInfo sourceInfo = s_instance->m_storeManager->getSource(sourceName);
    if (sourceInfo.name.isEmpty()) {
        return -ENOENT;
    }

    QDir sourceDir(sourceInfo.fullPath);
    if (!sourceDir.removeRecursively()) {
        return -EIO;
    }

    // 更新文件列表
    s_instance->m_fileList = s_instance->m_storeManager->getFlatFileList();
    return 0;
}

int ClericumFuse::fuseAccess(const char* path, int mask) {
    if (!s_instance) {
        return -ENOENT;
    }
    s_instance->refreshCache();

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

    QString realPath = s_instance->m_fileList.value(pathStr);

    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    QFileInfo fileInfo(realPath);
    if (!fileInfo.exists()) {
        return -ENOENT;
    }

    return 0;
}

int ClericumFuse::fuseReaddir(const char* path, void* buf,
    fuse_fill_dir_t filler,
    off_t offset,
    struct fuse_file_info* fi,
    enum fuse_readdir_flags flags) {
    Q_UNUSED(offset);
    Q_UNUSED(fi);
    Q_UNUSED(flags);

    if (!s_instance) {
        return -ENOENT;
    }
    s_instance->refreshCache();

    QString pathStr = QString::fromUtf8(path);

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
    for (const QString& virtualPath : s_instance->m_fileList.keys()) {
        filler(buf, qPrintable(virtualPath), nullptr, 0,
            static_cast<fuse_fill_dir_flags>(0));
    }

    return 0;
}

int ClericumFuse::fuseOpen(const char* path, struct fuse_file_info* fi) {
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

    QString realPath = s_instance->m_fileList.value(pathStr);

    if (realPath.isEmpty()) {
        return -ENOENT;
    }

    return 0;
}

int ClericumFuse::fuseRead(const char* path, char* buf, size_t size,
    off_t offset, struct fuse_file_info* fi) {
    Q_UNUSED(fi);

    if (!s_instance) {
        return -ENOENT;
    }
    s_instance->refreshCache();

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    // 检查是否是挂载点标记文件
    if (pathStr == MOUNT_MARKER_FILE) {
        QByteArray storePathData = s_instance->storePath().toUtf8();
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

    QString realPath = s_instance->m_fileList.value(pathStr);

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
    off_t offset, struct fuse_file_info* fi) {
    Q_UNUSED(fi);

    if (!s_instance) {
        return -ENOENT;
    }
    s_instance->refreshCache();

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    // 备份文件的写入重定向到本源文件
    if (s_instance->m_storeManager->isBackupFile(pathStr)) {
        // 使用 resolveRealPath 获取本源文件的 current 路径进行写入
        QString realPath = s_instance->m_storeManager->resolveRealPath(pathStr);
        if (realPath.isEmpty()) {
            return -ENOENT;
        }

        QFile file(realPath);
        if (!file.open(QIODevice::WriteOnly)) {
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

    QString realPath = s_instance->m_fileList.value(pathStr);

    if (realPath.isEmpty()) {
        return -ENOENT;
    }
    s_instance->refreshCache();

    QFile file(realPath);
    if (!file.open(QIODevice::WriteOnly)) {
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
    struct fuse_file_info* fi) {
    Q_UNUSED(mode);

    if (!s_instance) {
        return -ENOENT;
    }
    s_instance->refreshCache();

    QString pathStr = QString::fromUtf8(path);
    pathStr = pathStr.mid(1);  // 移除前导 /

    // 不允许创建备份文件
    if (s_instance->m_storeManager->isBackupFile(pathStr)) {
        return -EACCES;
    }

    // 创建本源文件
    QString sourceName = pathStr;

    // 确保本源文件条目存在
    if (!s_instance->m_storeManager->sourceExists(sourceName)) {
        s_instance->m_storeManager->createSource(sourceName);
    }

    QString realPath = s_instance->m_storeManager->getCurrentPath(sourceName);

    // 创建空文件
    QFile file(realPath);
    if (!file.open(QIODevice::WriteOnly)) {
        return -EIO;
    }
    file.close();

    // 更新文件列表
    s_instance->m_fileList = s_instance->m_storeManager->getFlatFileList();

    fi->flags |= O_WRONLY;
    return 0;
}

// FUSE 操作结构体（按 fuse_operations 定义顺序）
static struct fuse_operations clericFuseOps = {
    .getattr = ClericumFuse::fuseGetattr,
    .unlink = ClericumFuse::fuseUnlink,
    .open = ClericumFuse::fuseOpen,
    .read = ClericumFuse::fuseRead,
    .write = ClericumFuse::fuseWrite,
    .readdir = ClericumFuse::fuseReaddir,
    .access = ClericumFuse::fuseAccess,
    .create = ClericumFuse::fuseCreate,
};

bool ClericumFuse::mount() {
    if (m_status == MountStatus::Mounted) {
        qWarning() << "Already mounted";
        return false;
    }

    if (m_storeManager->storePath().isEmpty()) {
        qWarning() << "Store path not set";
        return false;
    }

    if (m_mountPath.isEmpty()) {
        qWarning() << "Mount path not set";
        return false;
    }

    if (!m_storeManager->isValidStore()) {
        qWarning() << "Invalid store:" << m_storeManager->storePath();
        return false;
    }

    // 创建挂载点目录
    QDir mountDir(m_mountPath);
    if (!mountDir.exists()) {
        if (!mountDir.mkpath(".")) {
            qWarning() << "Failed to create mount directory:" << m_mountPath;
            return false;
        }
    }

    m_status = MountStatus::Mounting;
    QCoreApplication::processEvents();

    // 刷新文件列表
    m_fileList = m_storeManager->getFlatFileList();

    // FUSE 参数 - 不使用调试模式 (-d)，fuse_main 会自动后台运行
    QByteArray mountPointBytes = m_mountPath.toUtf8();
    char* args[] = {
        const_cast<char*>(_PROJECT_NAME),
        mountPointBytes.data(),  // 挂载点
    };
    int argc = 2;

    // 在当前线程运行 FUSE 主循环
    // 不使用 -d 参数时，fuse_main 会自动 daemonize 并在后台运行
    int ret = fuse_main(argc, args, &clericFuseOps, nullptr);

    m_status = MountStatus::NotMounted;

    if (ret != 0) {
        qWarning() << "FUSE exited with code:" << ret;
        return false;
    }

    emit unmounted();
    return true;
}
