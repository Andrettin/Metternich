#include "metternich.h"

#include "engine_interface.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "game/game.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/scenario.h"
#include "util/container_util.h"
#include "util/exception_util.h"

namespace metternich {

engine_interface::engine_interface()
{
	connect(preferences::get(), &preferences::scale_factor_changed, this, &engine_interface::scale_factor_changed);
}

engine_interface::~engine_interface()
{
}

double engine_interface::get_scale_factor() const
{
	return preferences::get()->get_scale_factor().to_double();
}

defines *engine_interface::get_defines() const
{
	return defines::get();
}

game *engine_interface::get_game() const
{
	return game::get();
}

map *engine_interface::get_map() const
{
	return map::get();
}

preferences *engine_interface::get_preferences() const
{
	return preferences::get();
}

QObject *engine_interface::get_map_template(const QString &identifier) const
{
	try {
		return map_template::get(identifier.toStdString());
	} catch (const std::exception &exception) {
		exception::report(exception);
		return nullptr;
	}
}

QVariantList engine_interface::get_scenarios() const
{
	return container::to_qvariant_list(scenario::get_all());
}

}
