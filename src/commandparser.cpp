/**
 * @file commandcpp
 * @brief CommandParser 类实现
 */

#include "commandparser.h"
#include <QRegularExpression>

CommandParser::CommandParser(QObject* parent)
    : QObject{ parent } {
}

CommandParser::~CommandParser() {
}

void CommandParser::setApplication(const QString& description) {
    QString commandsDescription;
    QStringList commands;
    for (const QStringList& name : m_commands.keys()) {
        // 准备 commandsDescription
        const commandInfo info = m_commands.value(name);
        // 需要复制，避免修改原数据
        QStringList arguments = info.arguments;
        // "str" -> "<str>"
        arguments.replaceInStrings(QRegularExpression("(^.*$)"), "<\\1>");
        commandsDescription += "\n" + name.join(" ") + " " + arguments.join(" ") + "\t" + info.description;

        // 准备 commands
        commands.append(name.join(" "));
    }
    QCommandLineParser::setApplicationDescription(description + "\n"
        "Commands: " +
        commandsDescription);
    addPositionalArgument("command", "The command to execute: " + commands.join(", "));
}

CommandParser& CommandParser::registerCommand(const QStringList& name, const QStringList& arguments, const QString& description, executor executorFunction) {
    m_commands.insert(name, { arguments, description, executorFunction });
    return *this;
}

bool CommandParser::QStringListStartsWith(const QStringList& toCompare, const QStringList& starts) {
    for (int i = 0; i < starts.size(); i++) {
        if (toCompare.at(i) != starts.at(i)) {
            return false;
        }
    }
    return true;
}

int CommandParser::process(const QCoreApplication& app) {
    QCommandLineParser::process(app);

    const QStringList arguments = positionalArguments();
    if (arguments.isEmpty()) {
        showHelp(-1);
    }

    CommandHandler::Result result;
    // 创建命令处理器
    CommandHandler handler;

    bool foundCommand = false;
    for (const QStringList& name : m_commands.keys()) {
        if (QStringListStartsWith(arguments, name)) {
            const commandInfo info = m_commands.value(name);
            const QStringList neededArguments = info.arguments;

            if (name.length() + neededArguments.length() > arguments.length()) {
                qCritical() << name.join(" ") << "requires" << neededArguments.join(", ") << "arguments";
                showHelp(-1);
            }

            /**
             * @brief 传递了的参数
             */
            const QStringList passedArguments = arguments.sliced(name.length());
            result = info.executorFunction(passedArguments);
            foundCommand = true;
        }
    }
    if (!foundCommand) {
        qCritical() << "Unknown command: " << arguments.join(" ");
        showHelp(-1);
    }

    // 输出结果
    if (result.success) {
        if (!result.message.isEmpty()) {
            qInfo() << result.message;
        }
        return 0;
    } else {
        if (!result.error.isEmpty()) {
            qCritical() << "Error:" << result.message << result.error;
        }
        return -1;
    }
}
