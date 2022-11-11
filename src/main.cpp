#include "metternich.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_type.h"
#include "country/diplomacy_state.h"
#include "country/landed_title_tier.h"
#include "database/database.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "engine_interface.h"
#include "game/game.h"
#include "game/scenario.h"
#include "infrastructure/improvement.h"
#include "map/diplomatic_map_image_provider.h"
#include "map/map.h"
#include "map/map_grid_model.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_type.h"
#include "map/tile_image_provider.h"
#include "population/population_type.h"
#include "ui/icon_image_provider.h"
#include "ui/interface_image_provider.h"
#include "unit/military_unit_domain.h"
#include "util/empty_image_provider.h"
#include "util/event_loop.h"
#include "util/exception_util.h"
#include "util/gender.h"
#include "util/log_output_handler.h"
#include "util/log_util.h"
#include "util/path_util.h"
#include "util/thread_pool.h"

#include "maskedmousearea.h"

#pragma warning(push, 0)
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#pragma warning(pop)

using namespace metternich;

static void on_exit_cleanup()
{
	database::get()->clear();
}

int main(int argc, char **argv)
{
	try {
		const std::filesystem::path error_log_path = std::filesystem::current_path() / "error.log";
		const log_output_handler log_output_handler(error_log_path);

		qInstallMessageHandler(log::log_qt_message);

		QApplication app(argc, argv);
		app.setApplicationName("Metternich");
		app.setApplicationVersion("1.0.0");
		app.setOrganizationName("Metternich");
		app.setOrganizationDomain("andrettin.github.io");

		database::get()->set_defines(defines::get());

		QQmlApplicationEngine engine;

		enum_converter<country_type>();
		enum_converter<diplomacy_state>();
		enum_converter<gender>();
		enum_converter<landed_title_tier>();
		enum_converter<military_unit_domain>();
		enum_converter<site_type>();

		qmlRegisterAnonymousType<country>("", 1);
		qmlRegisterAnonymousType<country_game_data>("", 1);
		qmlRegisterAnonymousType<defines>("", 1);
		qmlRegisterAnonymousType<game>("", 1);
		qmlRegisterAnonymousType<improvement>("", 1);
		qmlRegisterAnonymousType<map>("", 1);
		qmlRegisterAnonymousType<population_type>("", 1);
		qmlRegisterAnonymousType<preferences>("", 1);
		qmlRegisterAnonymousType<province>("", 1);
		qmlRegisterAnonymousType<province_game_data>("", 1);
		qmlRegisterAnonymousType<scenario>("", 1);
		qmlRegisterAnonymousType<site>("", 1);
		qmlRegisterAnonymousType<site_game_data>("", 1);

		qmlRegisterType<map_grid_model>("map_grid_model", 1, 0, "MapGridModel");
		qmlRegisterType<MaskedMouseArea>("MaskedMouseArea", 1, 0, "MaskedMouseArea");

		engine.rootContext()->setContextProperty("metternich", engine_interface::get());

		engine.addImageProvider("diplomatic_map", new diplomatic_map_image_provider);
		engine.addImageProvider("empty", new empty_image_provider);
		engine.addImageProvider("icon", new icon_image_provider);
		engine.addImageProvider("interface", new interface_image_provider);
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

					//load the preferences before initializing the database, so that is any initialization depends on the scale factor, it can work properly
					preferences::get()->load();

					database::get()->initialize();
					engine_interface::get()->set_running(true);
				} catch (const std::exception &exception) {
					exception::report(exception);
					QMetaObject::invokeMethod(QApplication::instance(), [] {
						QApplication::exit(EXIT_FAILURE);
					}, Qt::QueuedConnection);
				}
			});
		}, Qt::QueuedConnection);

		engine.load(url);

		QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, []() {
			thread_pool::get()->stop();
			event_loop::get()->stop();
		});

		const int result = app.exec();

		on_exit_cleanup();

		return result;
	} catch (const std::exception &exception) {
		exception::report(exception);
		return -1;
	}
}
