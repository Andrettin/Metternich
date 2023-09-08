#include "metternich.h"

#include "map/site_game_data.h"

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
#include "map/tile.h"
#include "population/population_unit.h"
#include "script/context.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/map_util.h"

namespace metternich {

void site_game_data::reset_non_map_data()
{
	this->clear_population_units();
	this->settlement_type = nullptr;
	this->visiting_military_units.clear();
}

void site_game_data::do_turn()
{
	if (!this->get_visiting_military_units().empty()) {
		assert_throw(this->get_improvement() != nullptr);
		assert_throw(this->get_improvement()->is_ruins());

		const country *country = this->get_visiting_military_units().front()->get_country();
		context ctx(country);
		ctx.source_scope = this->site;
		ctx.attacking_military_units = this->get_visiting_military_units();
		country_event::check_events_for_scope(country, event_trigger::ruins_explored, ctx);

		//make the visiting units go back
		std::vector<military_unit *> visiting_military_units = this->get_visiting_military_units();

		for (military_unit *military_unit : visiting_military_units) {
			const province *province = military_unit->get_original_province();
			military_unit->set_original_province(nullptr);
			military_unit->set_site(nullptr);
			military_unit->set_province(province);
		}

		map::get()->set_tile_improvement(this->get_tile_pos(), nullptr);
	}
}

void site_game_data::set_tile_pos(const QPoint &tile_pos)
{
	if (tile_pos == this->get_tile_pos()) {
		return;
	}

	const province *old_province = this->get_province();

	this->tile_pos = tile_pos;
	emit tile_pos_changed();

	const province *province = this->get_province();

	if (old_province != province) {
		if (old_province != nullptr) {
			disconnect(old_province->get_game_data(), &province_game_data::owner_changed, this, &site_game_data::owner_changed);
		}

		if (province != nullptr) {
			connect(province->get_game_data(), &province_game_data::owner_changed, this, &site_game_data::owner_changed);
		}
	}
}

tile *site_game_data::get_tile() const
{
	if (this->get_tile_pos() != QPoint(-1, -1)) {
		return map::get()->get_tile(this->get_tile_pos());
	}

	return nullptr;
}

const province *site_game_data::get_province() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_province();
	}

	return nullptr;
}

bool site_game_data::is_provincial_capital() const
{
	if (this->get_province() == nullptr) {
		return false;
	}

	return this->get_province()->get_capital_settlement() == this->site;
}

bool site_game_data::is_capital() const
{
	if (this->get_province() == nullptr) {
		return false;
	}

	return this->get_province()->get_game_data()->is_capital() && this->is_provincial_capital();
}

const country *site_game_data::get_owner() const
{
	const province *province = this->get_province();
	if (province != nullptr) {
		return province->get_game_data()->get_owner();
	}

	return nullptr;
}

const culture *site_game_data::get_culture() const
{
	const province *province = this->get_province();
	if (province != nullptr) {
		return province->get_game_data()->get_culture();
	}

	return nullptr;
}

const std::string &site_game_data::get_current_cultural_name() const
{
	return this->site->get_cultural_name(this->get_culture());
}

const religion *site_game_data::get_religion() const
{
	const province *province = this->get_province();
	if (province != nullptr) {
		return province->get_game_data()->get_religion();
	}

	return nullptr;
}

const improvement *site_game_data::get_improvement() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_improvement();
	}

	return nullptr;
}

void site_game_data::add_population_unit(population_unit *population_unit)
{
	this->population_units.push_back(population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void site_game_data::remove_population_unit(population_unit *population_unit)
{
	std::erase(this->population_units, population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void site_game_data::clear_population_units()
{
	this->population_units.clear();
}

void site_game_data::calculate_commodity_outputs()
{
	commodity_map<int> outputs;

	const province *province = this->get_province();

	const improvement *improvement = this->get_improvement();
	if (improvement != nullptr) {
		if (improvement->get_output_commodity() != nullptr) {
			outputs[improvement->get_output_commodity()] = improvement->get_output_multiplier();
		}

		const resource *resource = this->get_tile()->get_resource();
		if (province != nullptr && resource != nullptr) {
			for (const auto &[commodity, resource_map] : province->get_game_data()->get_commodity_bonuses_per_improved_resources()) {
				const auto find_iterator = resource_map.find(resource);
				if (find_iterator != resource_map.end()) {
					outputs[commodity] += find_iterator->second;
				}
			}
		}
	}

	if (province != nullptr) {
		for (auto &[commodity, output] : outputs) {
			for (const auto &[threshold, bonus] : province->get_game_data()->get_commodity_bonus_for_tile_threshold_map(commodity)) {
				if (output >= threshold) {
					output += bonus;
				}
			}
		}
	}

	for (const auto &[commodity, output] : outputs) {
		this->set_commodity_output(commodity, output);
	}
}

void site_game_data::set_settlement_type(const metternich::settlement_type *settlement_type)
{
	if (settlement_type == this->get_settlement_type()) {
		return;
	}

	this->settlement_type = settlement_type;
	emit settlement_type_changed();

	if (game::get()->is_running()) {
		emit map::get()->tile_improvement_changed(this->get_tile_pos());
	}
}

QVariantList site_game_data::get_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_outputs());
}

void site_game_data::set_commodity_output(const commodity *commodity, const int output)
{
	assert_throw(output >= 0);

	const int old_output = this->get_commodity_output(commodity);
	if (output == old_output) {
		return;
	}

	if (output == 0) {
		this->commodity_outputs.erase(commodity);
	} else {
		this->commodity_outputs[commodity] = output;
	}

	const province *province = this->get_province();
	if (province != nullptr) {
		const country *owner = province->get_game_data()->get_owner();
		if (owner != nullptr) {
			owner->get_game_data()->change_commodity_output(commodity, output - old_output);
		}
	}

	emit commodity_outputs_changed();
}

}
