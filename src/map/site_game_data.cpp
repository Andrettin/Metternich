#include "metternich.h"

#include "map/site_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/tile.h"
#include "script/context.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/map_util.h"

namespace metternich {

void site_game_data::reset_non_map_data()
{
	this->visiting_military_units.clear();
}

void site_game_data::do_turn()
{
	if (!this->get_visiting_military_units().empty()) {
		assert_throw(this->get_improvement() != nullptr);
		assert_throw(this->get_improvement()->is_ruins());

		const country *country = this->get_visiting_military_units().front()->get_owner();
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

	for (const auto &[commodity, output] : outputs) {
		this->set_commodity_output(commodity, output);
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
