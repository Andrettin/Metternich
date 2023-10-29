#include "metternich.h"

#include "unit/army.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/context.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace metternich {

army::army(const std::vector<military_unit *> &military_units, target_variant &&target)
	: military_units(military_units), target(std::move(target))
{
	assert_throw(!this->get_military_units().empty());

	this->country = this->get_military_units().at(0)->get_country();

	for (military_unit *military_unit : this->get_military_units()) {
		military_unit->set_army(this);
	}

	const site *target_site = this->get_target_site();
	if (target_site != nullptr) {
		assert_throw(target_site->get_game_data()->get_improvement() != nullptr);
		assert_throw(target_site->get_game_data()->get_improvement()->is_ruins());
	}
}

void army::do_turn()
{
	if (this->get_target_province() != nullptr) {
		for (military_unit *military_unit : this->get_military_units()) {
			military_unit->set_province(this->get_target_province());

			//when ships move to a water zone, explore all adjacent water zones and coasts as well
			if (this->get_target_province()->is_water_zone()) {
				for (const province *neighbor_province : this->get_target_province()->get_game_data()->get_neighbor_provinces()) {
					if (this->get_country()->get_game_data()->is_province_explored(neighbor_province)) {
						continue;
					}

					if (neighbor_province->is_water_zone()) {
						this->get_country()->get_game_data()->explore_province(neighbor_province);
					} else {
						//for coastal provinces bordering the water zone, explore all their tiles bordering it
						for (const QPoint &coastal_tile_pos : neighbor_province->get_game_data()->get_border_tiles()) {
							if (!map::get()->is_tile_on_province_border_with(coastal_tile_pos, this->get_target_province())) {
								continue;
							}

							if (!this->get_country()->get_game_data()->is_tile_explored(coastal_tile_pos)) {
								this->get_country()->get_game_data()->explore_tile(coastal_tile_pos);
							}
						}
					}
				}
			}
		}
	} else if (this->get_target_site() != nullptr) {
		const site *target_site = this->get_target_site();
		site_game_data *target_site_game_data = target_site->get_game_data();
		assert_throw(target_site_game_data->get_improvement() != nullptr);
		assert_throw(target_site_game_data->get_improvement()->is_ruins());

		context ctx(this->get_country());
		ctx.source_scope = target_site;
		ctx.attacking_army = this;
		country_event::check_events_for_scope(country, event_trigger::ruins_explored, ctx);

		map::get()->set_tile_improvement(target_site_game_data->get_tile_pos(), nullptr);
	}
}

army::~army()
{
	for (military_unit *military_unit : this->get_military_units()) {
		assert_throw(military_unit->get_army() == this);
		military_unit->set_army(nullptr);
	}
}

QVariantList army::get_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_military_units());
}

void army::add_military_unit(military_unit *military_unit)
{
	if (vector::contains(this->get_military_units(), military_unit)) {
		throw std::runtime_error(std::format("Tried to add military unit \"{}\" to an army, but it is already a part of it.", military_unit->get_name()));
	}

	this->military_units.push_back(military_unit);

	military_unit->set_army(this);

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void army::remove_military_unit(military_unit *military_unit)
{
	if (!vector::contains(this->get_military_units(), military_unit)) {
		throw std::runtime_error(std::format("Tried to remove military unit \"{}\" from an army, but it is not a part of it.", military_unit->get_name()));
	}

	std::erase(this->military_units, military_unit);

	military_unit->set_army(nullptr);

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

int army::get_score() const
{
	int score = 0;

	for (const military_unit *military_unit : this->get_military_units()) {
		score += military_unit->get_score();
	}

	return score;
}

const character *army::get_commander() const
{
	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_character() == nullptr) {
			continue;
		}

		return military_unit->get_character();
	}

	return nullptr;
}

}
