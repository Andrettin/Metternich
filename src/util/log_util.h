#pragma once

namespace metternich::log {

constexpr const char *date_string_format = "yyyy.MM.dd hh:mm:ss";
constexpr uintmax_t max_size = 1000000; //1 MB

inline void log(const std::string_view &message)
{
	std::cout << "[" << QDateTime::currentDateTime().toString(log::date_string_format).toStdString() << "] " << message << '\n';
}

inline void log_error(const std::string_view &error_message)
{
	std::cerr << "[" << QDateTime::currentDateTime().toString(log::date_string_format).toStdString() << "] " << error_message << std::endl;
}

inline void log_qt_message(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	std::string log_message;

	switch (type) {
		case QtDebugMsg:
			log_message += "Debug";
			break;
		case QtInfoMsg:
			log_message += "Info";
			break;
		case QtWarningMsg:
			log_message += "Warning";
			break;
		case QtCriticalMsg:
			log_message += "Critical";
			break;
		case QtFatalMsg:
			log_message += "Fatal";
			break;
	}

	log_message += ": ";

	static const std::string default_category_name = "default";
	if (context.category != nullptr && context.category != default_category_name) {
		log_message += context.category;
		log_message += ": ";
	}

	log_message += msg.toStdString();

	if (context.file != nullptr) {
		log_message += " (";
		log_message += context.file;
		log_message += ": ";
		log_message += context.line;

		if (context.function != nullptr) {
			log_message += ", ";
			log_message += context.function;
		}

		log_message += ")";
	}

	switch (type) {
		case QtDebugMsg:
		case QtInfoMsg:
			log::log(log_message);
			break;
		case QtWarningMsg:
		case QtCriticalMsg:
		case QtFatalMsg:
			log::log_error(log_message);
			break;
	}
}

}
