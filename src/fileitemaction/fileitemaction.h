/**
 * @file fileitemaction.h
 * @brief ClericumFileItemAction 类声明
 *
 * 这是一个 KDE/Dolphin 的 FileItemAction 插件，在文件管理器的右键菜单中
 * 为 clericum 提供快捷操作：
 * - 在任意普通（非 store、非挂载点）文件夹上：创建 store / 挂载（load）
 * - 在挂载点上：卸载（unload）
 * - 在挂载点内的文件上：创建备份 / 加载备份 / 删除备份
 *
 * 实际命令通过直接复用 clericum_core 中的 CommandHandler 在 Dolphin 进程内
 * 完成，挂载点探测也复用 CommandHandler::findMountPoint，避免重复实现。
 * executeLoad 内部会以守护进程方式挂载 FUSE（父进程正常返回），因此在进程内
 * 直接调用是安全的。
 */

#pragma once

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class QAction;
class QWidget;

/**
 * @class ClericumFileItemAction
 * @brief clericum 的 KDE 文件项操作插件
 */
class ClericumFileItemAction : public KAbstractFileItemActionPlugin {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     * @param args KPlugin 参数
     */
    ClericumFileItemAction(QObject* parent, const QVariantList& args);

    /**
     * @brief 析构函数
     */
    ~ClericumFileItemAction() override;

    /**
     * @brief 根据选中的文件项返回右键菜单动作
     * @param fileItemInfos 选中文件项的属性
     * @param parentWidget 父窗口（用于弹出对话框）
     * @return 动作列表
     */
    QList<QAction*> actions(const KFileItemListProperties& fileItemInfos,
        QWidget* parentWidget) override;

private:
    /**
     * @brief 选择 store 并挂载到指定文件夹
     * @param mountPath 作为挂载点的目标文件夹路径
     * @param parentWidget 父窗口
     *
     * 在任意普通（非 store、非挂载点）文件夹上触发：弹出目录选择框让用户
     * 选择要挂载的 store，再将其挂载到 mountPath。
     */
    void mountStore(const QString& mountPath, QWidget* parentWidget);

    /**
     * @brief 在指定文件夹内创建新的 store
     * @param parentDir 在其下创建 store 的父文件夹路径
     * @param parentWidget 父窗口
     *
     * 在任意普通（非 store、非挂载点）文件夹上触发：弹出输入框让用户指定
     * store 名称，再在 parentDir 下创建名为该名称的 store。
     */
    void createStore(const QString& parentDir, QWidget* parentWidget);

    /**
     * @brief 卸载一个挂载点
     * @param mountPath 挂载点路径
     * @param parentWidget 父窗口
     */
    void unloadMount(const QString& mountPath, QWidget* parentWidget);

    /**
     * @brief 为指定虚拟文件创建备份
     * @param virtualPath 挂载点内的虚拟文件路径
     * @param parentWidget 父窗口
     */
    void createBackup(const QString& virtualPath, QWidget* parentWidget);
};
