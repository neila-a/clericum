/**
 * @file clericumfuse.cpp
 * @brief ClericumFuse 类实现
 */

#include "clericumfuse.h"

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

// FUSE 操作结构体（按 fuse_operations 定义顺序）
static fuse_operations clericFuseOps = {
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
    if (storePath().isEmpty()) {
        warn(i18n("Store path not set"));
        return false;
    }

    if (m_mountPath.isEmpty()) {
        warn(i18n("Mount path not set"));
        return false;
    }

    if (!m_storeManager->isValidStore()) {
        warn(i18n("Invalid store %1", storePath()));
        return false;
    }

    // 创建挂载点目录
    QDir mountDir(m_mountPath);
    if (!mountDir.exists()) {
        if (!mountDir.mkpath(".")) {
            warn(i18n("Failed to create mount directory %1", m_mountPath));
            return false;
        }
    }

    // 刷新文件列表
    getFileList() = m_storeManager->getFlatFileList();

    information(i18n("Mounted %1 at %2", storePath(), mountPath()));

    // FUSE 参数 - 不使用调试模式 (-d)，fuse_main 会自动后台运行
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    fuse_opt_add_arg(&args, _PROJECT_NAME);
    fuse_opt_add_arg(&args, m_mountPath.toUtf8().data());

    // 在当前线程运行 FUSE 主循环
    // 不使用 -d 参数时，fuse_main 会自动 daemonize 并在后台运行
    int ret = fuse_main(args.argc, args.argv, &clericFuseOps, nullptr);

    if (ret != 0) {
        warn(i18n("FUSE exited with code %1", ret));
        return false;
    }

    return true;
}
