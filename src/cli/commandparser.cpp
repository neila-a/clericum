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
        commandsDescription.append(QStringLiteral("\n%1 %2\t%3").arg(name.join(" "), arguments.join(" "), info.description));

        // 准备 commands
        commands.append(name.join(" "));
    }
    QCommandLineParser::setApplicationDescription(i18n("%1\n\nCommands: %2").arg(description, commandsDescription));
    addPositionalArgument("command", i18n("The command to execute: %1").arg(commands.join(", ")));
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
                critical(i18n("%1 requires %2 arguments", name.join(" "), neededArguments.join(", ")));
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
        critical(i18n("Unknown command %1", arguments.join(" ")));
        showHelp(-1);
    }

    // 输出结果
    if (result.success) {
        if (!result.message.isEmpty()) {
            information(result.message);
        }
        return 0;
    } else {
        critical(i18n("Error: %1", result.message));
        return -1;
    }
}
