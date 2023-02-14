#include "metternich.h"

#include "engine_interface.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "game/event_instance.h"
#include "game/game.h"
#include "game/scenario.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "time/era.h"
#include "unit/military_unit.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/vector_util.h"

namespace metternich {

engine_interface::engine_interface()
{
	connect(preferences::get(), &preferences::scale_factor_changed, this, &engine_interface::scale_factor_changed);
	connect(game::get(), &game::running_changed, this, [this]() {
		this->event_instances.clear();
		this->clear_selected_military_units();
	});
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

QVariantList engine_interface::get_eras() const
{
	return container::to_qvariant_list(era::get_all());
}

void engine_interface::add_event_instance(qunique_ptr<event_instance> &&event_instance)
{
	metternich::event_instance *event_instance_ptr = event_instance.get();
	this->event_instances.push_back(std::move(event_instance));
	emit event_fired(event_instance_ptr);
}

void engine_interface::remove_event_instance(event_instance *event_instance)
{
	emit event_closed(event_instance);

	std::erase_if(this->event_instances, [event_instance](const qunique_ptr<metternich::event_instance> &element) {
		return element.get() == event_instance;
	});
}

QVariantList engine_interface::get_selected_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_selected_military_units());
}

int engine_interface::get_selected_military_unit_category_count(const metternich::military_unit_category category)
{
	int count = 0;

	for (const military_unit *military_unit : this->get_selected_military_units()) {
		if (military_unit->get_category() == category) {
			++count;
		}
	}

	return count;
}

void engine_interface::change_selected_military_unit_category_count(const metternich::military_unit_category category, const int change, metternich::province *province)
{
	if (change == 0) {
		return;
	}

	if (change > 0) {
		const province_game_data *province_game_data = province->get_game_data();
		int added_count = 0;

		for (military_unit *military_unit : province_game_data->get_military_units()) {
			if (military_unit->get_owner() != game::get()->get_player_country()) {
				continue;
			}

			if (military_unit->get_category() != category) {
				continue;
			}

			if (vector::contains(this->get_selected_military_units(), military_unit)) {
				continue;
			}

			this->add_selected_military_unit(military_unit);

			++added_count;
			if (added_count == change) {
				break;
			}
		}

	} else {
		int removed_count = 0;

		const std::vector<military_unit *> military_units = this->get_selected_military_units();

		for (auto it = military_units.rbegin(); it != military_units.rend(); ++it) {
			const military_unit *military_unit = *it;
			if (military_unit->get_category() == category) {
				this->remove_selected_military_unit(military_unit);
				++removed_count;

				if (removed_count == std::abs(change)) {
					break;
				}
			}
		}
	}
}

}
