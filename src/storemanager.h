/**
 * @file storemanager.h
 * @brief StoreManager 类声明
 *
 * StoreManager 负责管理 clericum 的 store 文件夹结构，
 * 包括创建store、读取本源文件和备份文件等操作。
 *
 * @sa ClericumFuse
 */

#pragma once

#include <QObject>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QVector>
#include <QMap>
#include <QSharedPointer>
#include <QFile>

 /**
  * @brief 备份文件信息结构
  */
struct BackupInfo {
    QString name;           ///< 备份文件名
    QString fullPath;       ///< 完整路径
    QFileInfo fileInfo;     ///< 文件信息
};

/**
 * @brief 本源文件信息结构
 */
struct SourceInfo {
    QString name;           ///< 本源文件名
    QString fullPath;       ///< 本源文件夹完整路径
    QString currentPath;    ///< current 文件路径
    QString backupsPath;    ///< backups 文件夹路径
    QVector<BackupInfo> backups;  ///< 备份列表
};

/**
 * @class StoreManager
 * @brief 管理 clericum store 文件夹的类
 *
 * StoreManager 负责：
 * - 创建带有标记文件的 store 文件夹
 * - 读取和管理 store 中的本源文件和备份文件
 * - 验证 store 文件夹的有效性
 *
 * ### Store 文件夹结构 ###
 * @code
 * store/
 * ├── .clericum-store   # 标记文件
 * ├── filename1/
 * │   ├── current        # 本源文件内容
 * │   └── backups/
 * │       ├── backup1
 * │       └── backup2
 * └── filename2/
 *     ├── current
 *     └── backups/
 * @endcode
 *
 * ### 使用示例 ###
 * @code
 * StoreManager manager;
 * if (manager.create("/path/to/store")) {
 *     qDebug() << "Store created successfully";
 * }
 *
 * manager.setStorePath("/path/to/store");
 * auto sources = manager.getAllSources();
 * @endcode
 */
class StoreManager : public QObject {
    Q_OBJECT

public:
    /** @brief 标记文件名 */
    static constexpr const char* METADATA_FILENAME = ".clericum-store";
    /** @brief 本源文件内容文件名 */
    static constexpr const char* CURRENT_FILENAME = "current";
    /** @brief 备份文件夹名 */
    static constexpr const char* BACKUPS_DIRNAME = "backups";

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit StoreManager(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~StoreManager();

    /**
     * @brief 设置 store 文件夹路径
     * @param path store 文件夹路径
     */
    void setStorePath(const QString& path);

    /**
     * @brief 获取当前 store 文件夹路径
     * @return store 文件夹路径
     */
    QString storePath() const;

    /**
     * @brief 创建新的 store 文件夹
     * @param path 要创建的文件夹路径
     * @return 是否创建成功
     *
     * 创建一个新的 store 文件夹，包含：
     * - 元数据文件 (.clericum)
     * - 初始的空结构
     *
     * @code
     * StoreManager manager;
     * if (manager.create("/home/user/my-store")) {
     *     qDebug() << "Created successfully";
     * }
     * @endcode
     */
    bool create(const QString& path);

    /**
     * @brief 检查指定路径是否为有效的 store 文件夹
     * @param path 要检查的路径
     * @return 是否为有效的 store
     */
    bool isValidStore(const QString& path) const;

    /**
     * @brief 检查当前设置的 store 是否有效
     * @return 是否有效
     */
    bool isValidStore() const;

    /**
     * @brief 获取所有本源文件信息
     * @return 本源文件信息列表
     *
     * 扫描 store 文件夹，返回所有本源文件的信息。
     * 每个 SourceInfo 包含本源文件名、所有备份文件信息等。
     */
    QVector<SourceInfo> getAllSources() const;

    /**
     * @brief 根据文件名获取本源文件信息
     * @param name 本源文件名
     * @return 本源文件信息，如果不存在则返回空结构
     */
    SourceInfo getSource(const QString& name) const;

    /**
     * @brief 获取本源文件的 current 文件路径
     * @param name 本源文件名
     * @return current 文件完整路径
     */
    QString getCurrentPath(const QString& name) const;

    /**
     * @brief 创建新的本源文件条目
     * @param name 本源文件名
     * @return 是否创建成功
     *
     * 在 store 中创建一个新的本源文件条目：
     * - 创建本源文件夹
     * - 创建 current 文件
     * - 创建 backups 文件夹
     */
    bool createSource(const QString& name);

    /**
     * @brief 创建备份
     * @param sourceName 本源文件名
     * @param backupName 备份名
     * @return 是否创建成功
     *
     * 将本源文件的 current 内容复制到指定备份。
     */
    bool createBackup(const QString& sourceName, const QString& backupName);

    /**
     * @brief 从备份加载到 current
     * @param sourceName 本源文件名
     * @param backupName 备份名
     * @return 是否加载成功
     *
     * 将指定备份的内容复制到本源文件的 current 位置。
     */
    bool loadBackup(const QString& sourceName, const QString& backupName);

    /**
     * @brief 复制文件到 current
     * @param sourceName 本源文件名
     * @param sourceFile 要复制的源文件路径
     * @return 是否复制成功
     *
     * 将指定文件复制到本源文件的 current 位置。
     */
    bool copyToCurrent(const QString& sourceName, const QString& sourceFile);

    /**
     * @brief 检查本源文件是否存在
     * @param name 本源文件名
     * @return 是否存在
     */
    bool sourceExists(const QString& name) const;

    /**
     * @brief 获取 store 路径下的所有条目（平铺视图）
     * @return 文件名到文件路径的映射
     *
     * 返回一个映射，包含所有本源文件的 current 文件，
     * 以及所有备份文件（命名为 "备份名-本源名"）。
     *
     * 这个方法用于提供给 FUSE 文件系统显示平铺的文件列表。
     */
    QMap<QString, QString> getFlatFileList() const;

    /**
     * @brief 解析虚拟文件名
     * @param virtualName 虚拟文件名（可能是 "备份名-本源名" 或直接的本源名）
     * @param actualSourceName 输出：实际本源文件名
     * @param isBackup 输出：是否为备份文件
     * @return 是否解析成功
     *
     * 虚拟文件名的格式：
     * - 对于本源文件：直接是本源文件名
     * - 对于备份文件：是 "备份名-本源名" 格式
     */
    bool parseVirtualName(const QString& virtualName,
        QString& actualSourceName,
        bool& isBackup) const;

    /**
     * @brief 获取本源文件的真实路径
     * @param virtualName 虚拟文件名
     * @return 真实文件路径，如果是备份则返回本源文件的 current 路径
     */
    QString resolveRealPath(const QString& virtualName) const;

    /**
     * @brief 检查文件是否为备份文件
     * @param virtualName 虚拟文件名
     * @return 是否为备份文件
     */
    bool isBackupFile(const QString& virtualName) const;

    /**
     * @brief 获取 store 中的本源文件名列表
     * @return 本源文件名列表
     */
    QStringList getSourceNames() const;

    /**
     * @brief 刷新内部缓存
     */
    void refreshCache() const;

private:
    QString m_storePath;                    ///< store 文件夹路径
    mutable QMap<QString, SourceInfo> m_cache;  ///< 本源文件信息缓存
    mutable bool m_cacheValid = false;      ///< 缓存是否有效
};
