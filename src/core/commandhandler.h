/**
 * @file commandhandler.h
 * @brief CommandHandler 类声明
 *
 * CommandHandler 负责处理 clericum 的命令行命令，
 * 包括 store create/load/unload 和 backup create/load 命令。
 *
 * @sa ClericumFuse
 * @sa StoreManager
 */

#pragma once

#include "clericumfuse.h"

#include <expected>    // C++23 std::expected

 /**
  * @class CommandHandler
  * @brief 命令处理器类
  *
  * CommandHandler 管理所有 clericum 命令的执行：
  * - store create: 创建新的 store 文件夹
  * - store load: 挂载 FUSE 文件系统
  * - store unload: 卸载 FUSE 文件系统
  * - backup create: 创建文件备份
  * - backup load: 从备份加载到本源文件
  *
  * ### 命令使用示例 ###
  *
  * #### store create 命令 ####
  * ```cpp
  * CommandHandler handler;
  * handler.executeCreate("/path/to/store");
  * ```
  *
  * #### store load 命令 ####
  * ```cpp
  * CommandHandler handler;
  * handler.executeLoad("/path/to/store", "/path/to/mount");
  * ```
  *
  * #### backup create 命令 ####
  * ```cpp
  * handler.executeBackup("/path/to/mount/file", "backup-name");
  * ```
  *
  * #### backup load 命令 ####
  * ```cpp
  * handler.executeBackupLoad("/path/to/mount/file", "backup-name");
  * ```
  */
class CommandHandler : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 命令执行结果
     *
     * 用 C++23 std::expected 表达"值或错误"：QString 既作成功消息（值）
     * 也作失败消息（错误），与原有的 {bool success, QString message} 等价，
     * 但可借助 monadic 操作（and_then/or_else/transform）链式处理。
     * QString 可移动，满足 std::expected 的要求。
     */
    using Result = std::expected<QString, QString>;

    /**
     * @brief 挂载点信息
     */
    struct MountInfo {
        QString mountPath;        ///< 挂载点路径
        QString storePath;         ///< store 路径
    };

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit CommandHandler(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~CommandHandler();

    /**
     * @brief 执行 create 命令
     * @param path 要创建的 store 路径
     * @return 执行结果
     *
     * 在指定位置创建一个新的 store 文件夹，包含：
     * - 标记文件 (.clericum-store)
     * - 初始的空结构
     */
    Q_INVOKABLE Result executeCreate(const QString& path);

    /**
     * @brief 执行 load 命令
     * @param storePath store 文件夹路径
     * @param mountPath 挂载点路径
     * @return 执行结果
     *
     * 挂载一个 FUSE 文件系统到指定位置，展示 store 中的文件。
     */
    Q_INVOKABLE Result executeLoad(const QString& storePath, const QString& mountPath);

    /**
     * @brief 执行 unload 命令
     * @param mountPath 挂载点路径
     * @return 执行结果
     *
     * 卸载指定挂载点的 FUSE 文件系统。
     */
    Q_INVOKABLE Result executeUnload(const QString& mountPath);

    /**
     * @brief 执行 backup create 命令
     * @param virtualPath 虚拟文件路径（在 FUSE 文件系统中）
     * @param backupName 备份名称
     * @return 执行结果
     *
     * 创建指定文件的备份。
     * 虚拟路径格式：
     * - /mount/path/filename - 本源文件
     * - /mount/path/backupname-filename - 备份文件
     */
    Q_INVOKABLE Result executeBackup(const QString& virtualPath, const QString& backupName);

    /**
     * @brief 执行 backup load 命令
     * @param backupFile 备份文件路径（在 FUSE 文件系统中的完整路径）
     * @return 执行结果
     *
     * 从备份加载文件内容到本源文件。
     * 备份文件格式：
     * - /mount/path/backupname-filename - 备份文件
     */
    Q_INVOKABLE Result executeBackupLoad(const QString& backupFile);

    /**
     * @brief 执行 backup remove 命令
     * @param backupFile 备份文件路径（在 FUSE 文件系统中的完整路径）
     * @return 执行结果
     *
     * 删除指定文件的备份。
     */
    Q_INVOKABLE Result executeBackupRemove(const QString& backupFile);

    /**
     * @brief 检查路径是否在已挂载的文件系统中
     * @param path 要检查的路径
     * @return 是否处于某个挂载点之下
     *
     * 检查指定路径是否是某个挂载点的子路径。
     * 公开以便 KDE 文件项操作插件等复用，避免在多处重复挂载点探测逻辑。
     */
    Q_INVOKABLE bool isPathMounted(const QString& path) const;

    /**
     * @brief 根据路径查找挂载点信息
     * @param path 文件路径
     * @return 挂载点信息，如果不在任何挂载点中则返回空的 MountInfo
     *
     * 公开以便 KDE 文件项操作插件等复用，避免在多处重复挂载点探测逻辑。
     */
    Q_INVOKABLE MountInfo findMountPoint(const QString& path) const;

private:

    /**
     * @brief 从虚拟路径提取文件名
     * @param virtualPath 虚拟路径
     * @return 文件名（不含路径）
     */
    static QString extractFileName(const QString& virtualPath);

    /**
     * @brief 从虚拟路径提取本源文件名
     * @param virtualPath 虚拟路径
     * @return 本源文件名
     *
     * 对于备份文件（backupname-filename），返回 filename。
     */
    static QString extractSourceName(const QString& virtualPath);

    /**
     * @brief 从虚拟路径提取备份名
     * @param virtualPath 虚拟路径
     * @return 备份名
     *
     * 对于备份文件（backupname-filename），返回 backupname。
     */
    static QString extractBackupNameFromVirtualPath(const QString& virtualPath);

    /**
     * @brief 验证路径是否安全
     * @param path 要验证的路径
     * @return 是否安全
     *
     * 检查路径是否包含不安全的字符或模式。
     */
    static bool validatePath(const QString& path);

};
