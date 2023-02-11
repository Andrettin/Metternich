#include "metternich.h"

#include "map/site_game_data.h"

#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/tile.h"

namespace metternich {

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
