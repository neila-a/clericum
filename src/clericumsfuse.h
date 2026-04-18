/**
 * @file clericumsfuse.h
 * @brief ClericumFuse 类声明
 *
 * ClericumFuse 实现了一个 FUSE 文件系统，用于：
 * - 将 store 中的本源文件和备份文件以平铺的方式展示
 * - 对备份文件的写入操作重定向到本源文件
 *
 * @sa StoreManager
 */

#ifndef CLERICUMSFUSE_H
#define CLERICUMSFUSE_H

#include "storemanager.h"

#include <QObject>
#include <QString>
#include <QMap>
#include <QSharedPointer>
#include <QDir>

#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>

// 挂载点标记文件名
static const char* MOUNT_MARKER_FILE = ".clericum-mount";

/**
 * @class ClericumFuse
 * @brief FUSE 文件系统实现类
 *
 * ClericumFuse 将 store 文件夹中的内容以平铺的虚拟文件系统形式展示：
 * - 所有本源文件的 current 文件直接显示为文件名
 * - 所有备份文件显示为 "备份名-本源名" 格式
 * - 对备份文件的写入操作会被重定向到本源文件的 current 文件
 *
 * ### 虚拟文件系统结构 ###
 * @code
 * /mount/path/
 * ├── .clericum-mount          # 挂载点标记文件（内容为 store 路径）
 * ├── file1                    # 本源文件1的 current
 * ├── file2                    # 本源文件2的 current
 * ├── backup1-file1           # 本源文件1的 backup1 备份
 * └── backup2-file1           # 本源文件1的 backup2 备份
 * @endcode
 *
 * ### 使用示例 ###
 * @code
 * ClericumFuse fuse;
 * fuse.setStorePath("/path/to/store");
 * fuse.setMountPath("/path/to/mount");
 *
 * // 在另一个线程中运行
 * fuse.mount();
 * @endcode
 */
class ClericumFuse : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 挂载状态枚举
     */
    enum class MountStatus {
        NotMounted,    ///< 未挂载
        Mounting,      ///< 正在挂载
        Mounted,       ///< 已挂载
        Unmounting,    ///< 正在卸载
        Error          ///< 错误
    };
    Q_ENUM(MountStatus)

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ClericumFuse(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ClericumFuse();

    /**
     * @brief 设置 store 文件夹路径
     * @param path store 文件夹路径
     */
    void setStorePath(const QString &path);

    /**
     * @brief 获取 store 文件夹路径
     * @return store 路径
     */
    QString storePath() const;

    /**
     * @brief 设置挂载点路径
     * @param path 挂载点路径
     */
    void setMountPath(const QString &path);

    /**
     * @brief 获取挂载点路径
     * @return 挂载点路径
     */
    QString mountPath() const;

    /**
     * @brief 获取当前挂载状态
     * @return 挂载状态
     */
    MountStatus status() const;

    /**
     * @brief 挂载文件系统
     * @return 是否成功
     */
    bool mount();

    /**
     * @brief 刷新文件系统缓存
     *
     * 重新扫描 store 文件夹，更新虚拟文件列表。
     */
    void refreshCache();

    /**
     * @brief 获取文件名列表
     * @return 虚拟文件名列表
     */
    QStringList fileNames() const;

    /**
     * @brief 根据虚拟文件名获取真实路径
     * @param name 虚拟文件名
     * @return 真实文件路径
     */
    QString resolvePath(const QString &name) const;

    /**
     * @brief 检查是否为备份文件
     * @param name 虚拟文件名
     * @return 是否为备份文件
     */
    bool isBackupFile(const QString &name) const;

    /**
     * @brief 获取底层 StoreManager
     * @return StoreManager 指针
     */
    QSharedPointer<StoreManager> storeManager() const;

    // ============== FUSE 回调函数 ==============
    // 这些函数会被 FUSE 库调用

    /**
     * @brief 获取文件系统属性
     */
    static int fuseGetattr(const char *path, struct stat *stbuf,
                           struct fuse_file_info *fi);

    /**
     * @brief 访问文件
     */
    static int fuseAccess(const char *path, int mask);

    /**
     * @brief 读取目录
     */
    static int fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi,
                            enum fuse_readdir_flags flags);

    /**
     * @brief 打开文件
     */
    static int fuseOpen(const char *path, struct fuse_file_info *fi);

    /**
     * @brief 读取文件
     */
    static int fuseRead(const char *path, char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi);

    /**
     * @brief 写入文件
     */
    static int fuseWrite(const char *path, const char *buf, size_t size,
                         off_t offset, struct fuse_file_info *fi);

    /**
     * @brief 创建文件
     */
    static int fuseCreate(const char *path, mode_t mode,
                          struct fuse_file_info *fi);

signals:
    /**
     * @brief 挂载状态改变信号
     * @param status 新状态
     */
    void statusChanged(MountStatus status);

    /**
     * @brief 挂载成功信号
     */
    void mounted();

    /**
     * @brief 卸载完成信号
     */
    void unmounted();

    /**
     * @brief 错误信号
     * @param error 错误信息
     */
    void errorOccurred(const QString &error);

private:
    static ClericumFuse* s_instance;  ///< 静态实例指针，用于 FUSE 回调

    QSharedPointer<StoreManager> m_storeManager;  ///< Store 管理器
    QString m_mountPath;                          ///< 挂载点路径
    MountStatus m_status = MountStatus::NotMounted;  ///< 挂载状态
    QMap<QString, QString> m_fileList;            ///< 虚拟文件名到真实路径的映射
};

#endif // CLERICUMSFUSE_H
