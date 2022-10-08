#include "metternich.h"

#include "country/country_type.h"
#include "database/database.h"
#include "database/defines.h"
#include "engine_interface.h"
#include "map/map.h"
#include "map/site_type.h"
#include "map/tile_image_provider.h"
#include "util/event_loop.h"
#include "util/exception_util.h"
#include "util/log_util.h"
#include "util/path_util.h"

#pragma warning(push, 0)
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#pragma warning(pop)

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

static void on_exit_cleanup()
{
	database::get()->clear();
	clean_output();
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

		database::get()->set_defines(defines::get());

		QQmlApplicationEngine engine;

		enum_converter<country_type>();
		enum_converter<site_type>();

		qmlRegisterAnonymousType<defines>("", 1);
		qmlRegisterAnonymousType<map>("", 1);

		engine.rootContext()->setContextProperty("metternich", engine_interface::get());

		engine.addImageProvider("tile", new tile_image_provider);

		const QString root_path = path::to_qstring(database::get()->get_root_path());
		engine.addImportPath(root_path + "/libraries/qml");

		QUrl url = QDir(root_path + "/interface/").absoluteFilePath("Main.qml");
		url.setScheme("file");

		QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
			[url, argc, argv](QObject *obj, const QUrl &objUrl) {
			if (!obj && url == objUrl) {
				QCoreApplication::exit(-1);
			}

			event_loop::get()->co_spawn([argc, argv]() -> boost::asio::awaitable<void> {
				try {
					co_await database::get()->load(true);
					database::get()->load_defines();
					co_await database::get()->load(false);
					database::get()->initialize();
				} catch (const std::exception &exception) {
					exception::report(exception);
					QMetaObject::invokeMethod(QApplication::instance(), [] {
						QApplication::exit(EXIT_FAILURE);
					}, Qt::QueuedConnection);
				}
			});
		}, Qt::QueuedConnection);

		engine.load(url);

		const int result = app.exec();

		on_exit_cleanup();

		return result;
	} catch (const std::exception &exception) {
		exception::report(exception);
		return -1;
	}
}
