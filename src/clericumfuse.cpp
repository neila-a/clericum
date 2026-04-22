/**
 * @file clericumfuse.cpp
 * @brief ClericumFuse 类实现
 */

#include "clericumfuse.h"

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

QStringList ClericumFuse::fileNames() const {
    return getFileList().keys();
}

QString ClericumFuse::resolvePath(const QString& name) const {
    return getFileList().value(name, QString());
}

bool ClericumFuse::isBackupFile(const QString& name) const {
    return m_storeManager->isBackupFile(name);
}

QSharedPointer<StoreManager> ClericumFuse::storeManager() const {
    return m_storeManager;
}

// FUSE 操作结构体（按 fuse_operations 定义顺序）
static struct fuse_operations clericFuseOps = {
    .getattr = ClericumFuse::fuseGetattr,
    .unlink = ClericumFuse::fuseUnlink,
    .rename = ClericumFuse::fuseRename,
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
    getFileList() = m_storeManager->getFlatFileList();

    // FUSE 参数 - 不使用调试模式 (-d)，fuse_main 会自动后台运行
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    fuse_opt_add_arg(&args, _PROJECT_NAME);
    fuse_opt_add_arg(&args,  m_mountPath.toStdString().c_str());
    //fuse_opt_add_arg(&args, (QStringLiteral("-ostore=") + m_storeManager->storePath()).toStdString().c_str());

    // 在当前线程运行 FUSE 主循环
    // 不使用 -d 参数时，fuse_main 会自动 daemonize 并在后台运行
    int ret = fuse_main(args.argc, args.argv, &clericFuseOps, nullptr);

    m_status = MountStatus::NotMounted;

    if (ret != 0) {
        qWarning() << "FUSE exited with code:" << ret;
        return false;
    }

    return true;
}
