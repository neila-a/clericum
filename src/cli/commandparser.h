/**
 * @file commandparser.h
 * @brief CommandParser 类声明
 *
 * CommandParser 负责解析 clericum 的命令行命令，
 */

#pragma once

#include <QCommandLineParser>
#include <commandhandler.h>

using namespace std;
using executor = function<CommandHandler::Result(QStringList)>;

/**
 * @class CommandParser
 * @brief 命令解析器类
 */
class CommandParser : public QObject, public QCommandLineParser {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit CommandParser(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~CommandParser();

    /**
     * @brief 返回的是整个应用的返回值
     */
    int process(const QCoreApplication& app);

    /**
     * @brief 设置描述等内容
     * 只能执行一次。
     */
    void setApplication(const QString& description);

    /**
     * @brief 注册命令。
     * 可以链式调用。
     */
    CommandParser& registerCommand(const QStringList& name, const QStringList& arguments, const QString& description, executor executorFunction);

private:

    /**
     * @brief 判断一个 QStringList 是否由另一个 QStringList 开始。
     */
    static bool QStringListStartsWith(const QStringList& toCompare, const QStringList& starts);

    /**
     * @brief 命令信息
     * 也就是 registerCommand 的后几个参数。
     */
    struct commandInfo {
        QStringList arguments;
        QString description;
        executor executorFunction;
    };

    /**
     * @brief 由 registerCommand 记录。
     */
    QMap<QStringList, commandInfo> m_commands = {};
};
