#include "metternich.h"

#include "map/site_game_data.h"

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

namespace metternich {

void site_game_data::do_turn()
{
	if (!this->get_visiting_military_units().empty()) {
		assert_throw(this->get_improvement() != nullptr);
		assert_throw(this->get_improvement()->is_ruins());

		const country *country = this->get_visiting_military_units().front()->get_owner();
		read_only_context ctx(country);
		ctx.source_scope = this->site;
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

const improvement *site_game_data::get_improvement() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_improvement();
	}

	return nullptr;
}

int site_game_data::get_employee_count() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_employee_count();
	}

	return 0;
}

int site_game_data::get_employment_capacity() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_employment_capacity();
	}

	return 0;
}

int site_game_data::get_production_modifier() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return (tile->get_output_multiplier() * 100).to_int() - 100;
	}

	return 0;
}

}
