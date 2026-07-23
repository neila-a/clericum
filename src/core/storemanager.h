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

#include <QDir>

#include <KLocalizedString>

// Qt 优先的日志宏：保留 qInfo/qWarning/qCritical 等 Qt 设施，仅用可变参数宏简化调用。
#define log(type, ...) q##type().noquote() << __VA_ARGS__
#define warn(...) log(Warning, __VA_ARGS__)
#define information(...) log(Info, __VA_ARGS__)
#define critical(...) log(Critical, __VA_ARGS__)

// 备份名与本源名之间的分隔符。使用 inline 变量（C++17）避免潜在的 ODR 问题，
// 并改用 QStringView（C++17/Qt6 轻量视图）减少不必要的字符串拷贝。
inline constexpr QStringView separator = u" - ";

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
    QString currentPath;    ///< 本源文件路径（files/下的文件）
    QString backupsPath;    ///< backups/本源名 文件夹路径
    QList<BackupInfo> backups;  ///< 备份列表
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
 * ```
 * store/
 * ├── .clericum-store   # 标记文件
 * ├── files/
 * │   ├── filename1     # 本源文件内容
 * │   └── filename2
 * └── backups/
 *     ├── filename1/
 *     │   ├── backup1
 *     │   └── backup2
 *     └── filename2/
 *         └── backup3
 * ```
 *
 * ### 使用示例 ###
 * ```cpp
 * StoreManager manager;
 * if (manager.create("/path/to/store")) {
 *     // Store created successfully
 * }
 *
 * manager.setStorePath("/path/to/store");
 * ```
 */

// 不要用 using 或 typedef
#define filename static constexpr const char*

class StoreManager : public QObject {

    Q_OBJECT;
    Q_PROPERTY(QString storePath READ storePath WRITE setStorePath);

public:

    /** @brief 标记文件名 */
    filename METADATA_FILENAME = ".clericum-store";
    /** @brief 本源文件目录名 */
    filename FILES_DIRNAME = "files";
    /** @brief 备份文件夹名 */
    filename BACKUPS_DIRNAME = "backups";

    /**
     * @brief 删除备份文件
     * @param sourceName 本源文件名
     * @param backupName 备份名称
     * @return 是否成功删除
     */
    bool removeBackup(const QString& sourceName, const QString& backupName);

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
    [[nodiscard]] QString storePath() const;

    /**
     * @brief 创建新的 store 文件夹
     * @param path 要创建的文件夹路径
     * @return 是否创建成功
     *
     * 创建一个新的 store 文件夹，包含：
     * - 元数据文件 (.clericum)
     * - 初始的空结构
     *
     * ```cpp
     * StoreManager manager;
     * if (manager.create("/home/user/my-store")) {
     *     // Created successfully
     * }
     * ```
     */
    bool create();

    /**
     * @brief 检查当前设置的 store 是否有效
     * @return 是否有效
     */
    [[nodiscard]] bool isValidStore() const;

    /**
     * @brief 根据文件名获取本源文件信息
     * @param name 本源文件名
     * @return 本源文件信息，如果不存在则返回空结构
     */
    [[nodiscard]] SourceInfo getSource(const QString& name) const;

    /**
     * @brief 获取本源文件的 current 文件路径
     * @param name 本源文件名
     * @return current 文件完整路径
     */
    [[nodiscard]] QString getCurrentPath(const QString& name) const;

    /**
     * @brief 创建新的本源文件条目
     * @param name 本源文件名
     * @return 是否创建成功
     *
     * 在 store 中创建一个新的本源文件条目：
     * - 在 files/ 下创建本源文件
     * - 在 backups/ 下创建对应的备份子目录
     */
    bool createSource(const QString& name);

    /**
     * @brief 创建备份
     * @param sourceName 本源文件名
     * @param backupName 备份名
     * @return 是否创建成功
     *
     * 将本源文件的内容复制到指定备份。
     */
    bool createBackup(const QString& sourceName, const QString& backupName);

    /**
     * @brief 从备份加载到本源文件
     * @param sourceName 本源文件名
     * @param backupName 备份名
     * @return 是否加载成功
     *
     * 将指定备份的内容复制到本源文件。
     */
    bool loadBackup(const QString& sourceName, const QString& backupName);

    /**
     * @brief 检查本源文件是否存在
     * @param name 本源文件名
     * @return 是否存在
     */
    [[nodiscard]] bool sourceExists(const QString& name) const;

    /**
     * @brief 获取 store 路径下的所有条目（平铺视图）
     * @return 文件名到文件路径的映射
     *
     * 返回一个映射，包含所有本源文件的 current 文件，
     * 以及所有备份文件（命名为 "备份名-本源名"）。
     *
     * 这个方法用于提供给 FUSE 文件系统显示平铺的文件列表。
     */
    [[nodiscard]] QMap<QString, QString> getFlatFileList() const;

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
    [[nodiscard]] bool parseVirtualName(const QString& virtualName,
        QString& actualSourceName,
        bool& isBackup) const;

    /**
     * @brief 获取本源文件的真实路径
     * @param virtualName 虚拟文件名
     * @return 真实文件路径，如果是备份则返回本源文件路径
     */
    [[nodiscard]] QString resolveRealPath(const QString& virtualName) const;

    /**
     * @brief 检查文件是否为备份文件
     * @param virtualName 虚拟文件名
     * @return 是否为备份文件
     */
    [[nodiscard]] bool isBackupFile(const QString& virtualName) const;

    /**
     * @brief 获取 store 路径下的本源文件映射（内部使用）
     * @return 本源文件名到 SourceInfo 的映射
     */
    [[nodiscard]] QMap<QString, SourceInfo> scanSources() const;

private:
    QString m_storePath;                    ///< store 文件夹路径
};
