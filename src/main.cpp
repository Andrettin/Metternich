#include "pch.h"
#include "util/log_util.h"

#include <QApplication>
#include <QDir>
#include <QQmlApplicationEngine>

#include <filesystem>
#include <stdexcept>

using namespace metternich;

static void init_output()
{
	const std::filesystem::path error_log_path = std::filesystem::current_path() / "error.log";

	if (std::filesystem::exists(error_log_path) && std::filesystem::file_size(error_log_path) > 1000000) {
		std::filesystem::remove(error_log_path);
	}

	const std::string path_str = error_log_path.string();

	const FILE *error_log_file = freopen(path_str.c_str(), "a", stderr);
	if (error_log_file == nullptr) {
		log::log_error("Failed to create error log.");
	}
}

static void clean_output()
{
	std::cerr.clear();
	fclose(stderr);

	const std::filesystem::path error_log_path = std::filesystem::current_path() / "error.log";

	if (std::filesystem::exists(error_log_path) && std::filesystem::file_size(error_log_path) == 0) {
		std::filesystem::remove(error_log_path);
	}
}

int main(int argc, char **argv)
{
	try {
		init_output();

		qInstallMessageHandler(log::log_qt_message);

		QApplication app(argc, argv);
		app.setApplicationName("Metternich");
		app.setApplicationVersion("1.0.0");
		app.setOrganizationName("Metternich");
		app.setOrganizationDomain("andrettin.github.io");

		QQmlApplicationEngine engine;

		//const QString root_path = path::to_qstring(database::get()->get_root_path());
		const QString root_path = QString::fromStdString(std::filesystem::current_path().string());
		engine.addImportPath(root_path + "/libraries/qml");

		QUrl url = QDir(root_path + "/interface/").absoluteFilePath("Main.qml");
		url.setScheme("file");

		QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
			[url, argc, argv](QObject *obj, const QUrl &objUrl) {
			if (!obj && url == objUrl) {
				QCoreApplication::exit(-1);
			}
		}, Qt::QueuedConnection);

		engine.load(url);

		const int result = app.exec();

		clean_output();

		return result;
	} catch (const std::exception &exception) {
		//exception::report(exception);
		return -1;
	}
}
