#include "metternich.h"

#include "character/advisor_category.h"
#include "character/character.h"
#include "character/character_attribute.h"
#include "character/character_game_data.h"
#include "character/character_role.h"
#include "character/dynasty.h"
#include "character/trait_type.h"
#include "country/consulate.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_tier.h"
#include "country/country_tier_data.h"
#include "country/country_turn_data.h"
#include "country/country_type.h"
#include "country/cultural_group_rank.h"
#include "country/diplomacy_state.h"
#include "country/government_type.h"
#include "country/journal_entry.h"
#include "country/law.h"
#include "country/law_group.h"
#include "country/policy.h"
#include "database/database.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/food_type.h"
#include "economy/production_type.h"
#include "engine_interface.h"
#include "game/event.h"
#include "game/event_instance.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/game_rules.h"
#include "game/scenario.h"
#include "infrastructure/building_type.h"
#include "infrastructure/country_building_slot.h"
#include "infrastructure/improvement.h"
#include "infrastructure/improvement_slot.h"
#include "infrastructure/wonder.h"
#include "map/diplomatic_map_image_provider.h"
#include "map/elevation_type.h"
#include "map/forestation_type.h"
#include "map/map.h"
#include "map/map_country_model.h"
#include "map/map_grid_model.h"
#include "map/map_province_model.h"
#include "map/map_site_model.h"
#include "map/map_template.h"
#include "map/moisture_type.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_type.h"
#include "map/temperature_type.h"
#include "map/tile_image_provider.h"
#include "population/population.h"
#include "population/population_type.h"
#include "script/scripted_character_modifier.h"
#include "technology/technology.h"
#include "time/era.h"
#include "ui/icon.h"
#include "ui/icon_image_provider.h"
#include "ui/interface_image_provider.h"
#include "ui/portrait.h"
#include "ui/portrait_image_provider.h"
#include "unit/military_unit_domain.h"
#include "util/empty_image_provider.h"
#include "util/exception_util.h"
#include "util/gender.h"
#include "util/log_output_handler.h"
#include "util/log_util.h"
#include "util/path_util.h"

#include "maskedmousearea.h"

#pragma warning(push, 0)
#include "qcoro/qml/qcoroqml.h"

#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#pragma warning(pop)

using namespace metternich;

static QCoro::Task<void> initialize()
{
	try {
		co_await database::get()->load(true);
		database::get()->load_defines();
		co_await database::get()->load(false);

		//load the preferences before initializing the database, so that is any initialization depends on the scale factor, it can work properly
		preferences::get()->load();

		database::get()->initialize();
		engine_interface::get()->set_running(true);
	} catch (...) {
		exception::report(std::current_exception());
		QMetaObject::invokeMethod(QApplication::instance(), [] {
			QApplication::exit(EXIT_FAILURE);
		}, Qt::QueuedConnection);
	}
}

static void on_exit_cleanup()
{
	database::get()->clear();
}

int main(int argc, char **argv)
{
	try {
		const std::filesystem::path output_log_path = std::filesystem::current_path() / "output.log";
		const std::filesystem::path error_log_path = std::filesystem::current_path() / "error.log";
		const log_output_handler log_output_handler(output_log_path, error_log_path);

		qInstallMessageHandler(log::log_qt_message);

#ifdef Q_OS_WINDOWS
		qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");
#endif

		QApplication app(argc, argv);
		app.setApplicationName("Iron Barons");
		app.setApplicationVersion("1.0.0");
		app.setOrganizationName("Iron Barons");
		app.setOrganizationDomain("andrettin.github.io");

		database::get()->set_defines(defines::get());

		QQmlApplicationEngine engine;

		enum_converter<advisor_category>();
		enum_converter<character_attribute>();
		enum_converter<character_role>();
		enum_converter<country_tier>();
		enum_converter<country_type>();
		enum_converter<cultural_group_rank>();
		enum_converter<diplomacy_state>();
		enum_converter<elevation_type>();
		enum_converter<event_trigger>();
		enum_converter<food_type>();
		enum_converter<forestation_type>();
		enum_converter<gender>();
		enum_converter<improvement_slot>();
		enum_converter<log_level>();
		enum_converter<military_unit_domain>();
		enum_converter<moisture_type>();
		enum_converter<site_type>();
		enum_converter<temperature_type>();
		enum_converter<trait_type>();

		QCoro::Qml::registerTypes();

		qmlRegisterAnonymousType<building_type>("", 1);
		qmlRegisterAnonymousType<character>("", 1);
		qmlRegisterAnonymousType<character_game_data>("", 1);
		qmlRegisterAnonymousType<consulate>("", 1);
		qmlRegisterAnonymousType<country>("", 1);
		qmlRegisterAnonymousType<country_building_slot>("", 1);
		qmlRegisterAnonymousType<country_game_data>("", 1);
		qmlRegisterAnonymousType<country_tier_data>("", 1);
		qmlRegisterAnonymousType<country_turn_data>("", 1);
		qmlRegisterAnonymousType<defines>("", 1);
		qmlRegisterAnonymousType<dynasty>("", 1);
		qmlRegisterAnonymousType<era>("", 1);
		qmlRegisterAnonymousType<event>("", 1);
		qmlRegisterAnonymousType<event_instance>("", 1);
		qmlRegisterAnonymousType<game>("", 1);
		qmlRegisterAnonymousType<game_rules>("", 1);
		qmlRegisterAnonymousType<government_type>("", 1);
		qmlRegisterAnonymousType<icon>("", 1);
		qmlRegisterAnonymousType<improvement>("", 1);
		qmlRegisterAnonymousType<journal_entry>("", 1);
		qmlRegisterAnonymousType<law>("", 1);
		qmlRegisterAnonymousType<law_group>("", 1);
		qmlRegisterAnonymousType<map>("", 1);
		qmlRegisterAnonymousType<map_template>("", 1);
		qmlRegisterAnonymousType<policy>("", 1);
		qmlRegisterAnonymousType<population>("", 1);
		qmlRegisterAnonymousType<population_type>("", 1);
		qmlRegisterAnonymousType<portrait>("", 1);
		qmlRegisterAnonymousType<preferences>("", 1);
		qmlRegisterAnonymousType<production_type>("", 1);
		qmlRegisterAnonymousType<province>("", 1);
		qmlRegisterAnonymousType<province_game_data>("", 1);
		qmlRegisterAnonymousType<scenario>("", 1);
		qmlRegisterAnonymousType<scripted_character_modifier>("", 1);
		qmlRegisterAnonymousType<site>("", 1);
		qmlRegisterAnonymousType<site_game_data>("", 1);
		qmlRegisterAnonymousType<technology>("", 1);
		qmlRegisterAnonymousType<wonder>("", 1);

		qmlRegisterType<map_country_model>("map_country_model", 1, 0, "MapCountryModel");
		qmlRegisterType<map_grid_model>("map_grid_model", 1, 0, "MapGridModel");
		qmlRegisterType<map_province_model>("map_province_model", 1, 0, "MapProvinceModel");
		qmlRegisterType<map_site_model>("map_site_model", 1, 0, "MapSiteModel");
		qmlRegisterType<MaskedMouseArea>("MaskedMouseArea", 1, 0, "MaskedMouseArea");

		engine.rootContext()->setContextProperty("metternich", engine_interface::get());

		engine.addImageProvider("diplomatic_map", new diplomatic_map_image_provider);
		engine.addImageProvider("empty", new empty_image_provider);
		engine.addImageProvider("icon", new icon_image_provider);
		engine.addImageProvider("interface", new interface_image_provider);
		engine.addImageProvider("portrait", new portrait_image_provider);
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

			initialize();
		}, Qt::QueuedConnection);

		engine.load(url);

		const int result = app.exec();

		on_exit_cleanup();

		return result;
	} catch (...) {
		exception::report(std::current_exception());
		return -1;
	}
}
