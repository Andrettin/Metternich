#include "metternich.h"

#include "engine_interface.h"

#include "country/consulate.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_tier.h"
#include "country/country_tier_data.h"
#include "country/law_group.h"
#include "country/policy.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "game/event_instance.h"
#include "game/game.h"
#include "game/scenario.h"
#include "map/map.h"
#include "map/map_template.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "map/world.h"
#include "technology/technology.h"
#include "time/era.h"
#include "unit/army.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/qunique_ptr.h"
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

const map_template *engine_interface::get_map_template(const QString &identifier) const
{
	return map_template::try_get(identifier.toStdString());
}

const world *engine_interface::get_world(const QString &identifier) const
{
	return world::try_get(identifier.toStdString());
}

QVariantList engine_interface::get_scenarios() const
{
	std::vector<const scenario *> available_scenarios;

	for (const scenario *scenario : scenario::get_all()) {
		if (scenario->is_hidden()) {
			continue;
		}

		available_scenarios.push_back(scenario);
	}

	return container::to_qvariant_list(available_scenarios);
}

QVariantList engine_interface::get_major_scenarios() const
{
	std::vector<const scenario *> available_scenarios;

	for (const scenario *scenario : scenario::get_all()) {
		if (scenario->is_hidden()) {
			continue;
		}

		if (!scenario->is_major()) {
			continue;
		}

		available_scenarios.push_back(scenario);
	}

	return container::to_qvariant_list(available_scenarios);
}

QVariantList engine_interface::get_eras() const
{
	return container::to_qvariant_list(era::get_all());
}

QVariantList engine_interface::get_law_groups() const
{
	return container::to_qvariant_list(law_group::get_all());
}

QVariantList engine_interface::get_policies() const
{
	return container::to_qvariant_list(policy::get_all());
}

QVariantList engine_interface::get_technologies() const
{
	return container::to_qvariant_list(technology::get_all());
}

const country_tier_data *engine_interface::get_country_tier_data(const metternich::country_tier tier) const
{
	try {
		return country_tier_data::get(tier);
	} catch (...) {
		exception::report(std::current_exception());
		QApplication::exit(EXIT_FAILURE);
		return nullptr;
	}
}

const consulate *engine_interface::get_consulate(const QString &identifier) const
{
	return consulate::try_get(identifier.toStdString());
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
			if (military_unit->get_country() != game::get()->get_player_country()) {
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

void engine_interface::move_selected_military_units_to(const QPoint &tile_pos)
{
	assert_throw(!this->get_selected_military_units().empty());

	const tile *tile = map::get()->get_tile(tile_pos);

	army::target_variant target = std::monostate();

	if (tile->get_province() != nullptr) {
		if (tile->get_province() == this->get_selected_military_units().front()->get_province()) {
			const site *site = tile->get_site();
			if (site != nullptr && site->get_game_data()->can_be_visited()) {
				target = site;
			}
		} else {
			bool can_move = true;
			for (military_unit *military_unit : this->get_selected_military_units()) {
				if (!military_unit->can_move_to(tile->get_province())) {
					can_move = false;
					break;
				}
			}

			if (can_move) {
				target = tile->get_province();
			}
		}
	}

	if (!std::holds_alternative<std::monostate>(target)) {
		auto army = make_qunique<metternich::army>(this->get_selected_military_units(), std::move(target));
		this->get_selected_military_units().front()->get_country()->get_game_data()->add_army(std::move(army));
	}

	this->clear_selected_military_units();
}

}
