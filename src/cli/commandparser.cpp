/**
 * @file commandcpp
 * @brief CommandParser 类实现
 */

#include "commandparser.h"
#include <QRegularExpression>

#include <algorithm>   // C++23 std::ranges::starts_with / contains
#include <ranges>      // C++20 范围库
#include <utility>     // std::pair（fold_left 累加器）

CommandParser::CommandParser(QObject* parent)
    : QObject{ parent } {
}

CommandParser::~CommandParser() {
}

void CommandParser::setApplication(const QString& description) {
    // C++23 std::ranges::fold_left + Qt6 QMap::asKeyValueRange 结构化绑定遍历，
    // 消除 m_commands.value(name) 的冗余二次查找。
    const auto [commandsDescription, commands] = std::ranges::fold_left(
        m_commands.asKeyValueRange(),
        std::pair<QString, QStringList>{},
        [](std::pair<QString, QStringList> acc, const auto& entry) {
            const QStringList& name = entry.first;
            const commandInfo& info = entry.second;
            // 需要复制，避免修改原数据
            QStringList arguments = info.arguments;
            // "str" -> "<str>"
            arguments.replaceInStrings(QRegularExpression("(^.*$)"), "<\\1>");
            acc.first.append(QStringLiteral("\n%1 %2\t%3").arg(name.join(" "), arguments.join(" "), info.description));
            acc.second.append(name.join(" "));
            return acc;
        });

    QCommandLineParser::setApplicationDescription(i18n("%1\n\nCommands: %2").arg(description, commandsDescription));
    addPositionalArgument(i18n("command"), i18n("The command to execute: %1").arg(commands.join(", ")));
}

CommandParser& CommandParser::registerCommand(const QStringList& name, const QStringList& arguments, const QString& description, executor executorFunction) {
    m_commands.insert(name, { arguments, description, executorFunction });
    return *this;
}

bool CommandParser::QStringListStartsWith(const QStringList& toCompare, const QStringList& starts) {
    // 注：libstdc++ 15 尚未提供 C++23 的 std::ranges::starts_with，
    // 这里用 C++20 std::ranges::mismatch 实现等价语义：starts 耗尽即表示 toCompare 以其开头。
    const auto [it1, it2] = std::ranges::mismatch(toCompare, starts);
    return it2 == std::ranges::end(starts);
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
    for (const auto& [name, info] : m_commands.asKeyValueRange()) {
        if (QStringListStartsWith(arguments, name)) {
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

    // 输出结果（std::expected：has_value 表示成功，error() 为失败消息）
    if (result.has_value()) {
        if (!result->isEmpty()) {
            information(*result);
        }
        return 0;
    } else {
        critical(i18n("Error: %1", result.error()));
        return -1;
    }
}
