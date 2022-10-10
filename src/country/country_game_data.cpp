#include "metternich.h"

#include "country/country_game_data.h"

#include "country/country.h"
#include "country/diplomacy_state.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "util/vector_util.h"

namespace metternich {

void country_game_data::add_province(const province *province)
{
	this->provinces.push_back(province);

	const map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	for (const metternich::province *border_province : province_game_data->get_border_provinces()) {
		const metternich::province_game_data *border_province_game_data = border_province->get_game_data();
		if (border_province_game_data->get_owner() != this->country) {
			continue;
		}

		for (const QPoint &tile_pos : border_province_game_data->get_border_tiles()) {
			if (!map->is_tile_on_country_border(tile_pos)) {
				std::erase(this->border_tiles, tile_pos);
			}
		}
	}

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		if (map->is_tile_on_country_border(tile_pos)) {
			this->border_tiles.push_back(tile_pos);
		}
	}
}

void country_game_data::remove_province(const province *province)
{
	std::erase(this->provinces, province);

	const map *map = map::get();
	const province_game_data *province_game_data = province->get_game_data();

	for (const QPoint &tile_pos : province_game_data->get_border_tiles()) {
		std::erase(this->border_tiles, tile_pos);
	}

	for (const metternich::province *border_province : province_game_data->get_border_provinces()) {
		const metternich::province_game_data *border_province_game_data = border_province->get_game_data();
		if (border_province_game_data->get_owner() != this->country) {
			continue;
		}

		for (const QPoint &tile_pos : border_province_game_data->get_border_tiles()) {
			if (map->is_tile_on_country_border(tile_pos) && !vector::contains(this->get_border_tiles(), tile_pos)) {
				this->border_tiles.push_back(tile_pos);
			}
		}
	}
}

diplomacy_state country_game_data::get_diplomacy_state(const metternich::country *other_country) const
{
	const auto find_iterator = this->diplomacy_states.find(other_country);

	if (find_iterator != this->diplomacy_states.end()) {
		return find_iterator->second;
	}

	return diplomacy_state::peace;
}

void country_game_data::set_diplomacy_state(const metternich::country *other_country, const diplomacy_state state)
{
	if (is_vassalage_diplomacy_state(state)) {
		this->overlord = other_country;
	} else {
		if (this->overlord == other_country) {
			this->overlord = nullptr;
		}
	}

	if (state == diplomacy_state::peace) {
		this->diplomacy_states.erase(other_country);
	} else {
		this->diplomacy_states[other_country] = state;
	}
}

const QColor &country_game_data::get_diplomatic_map_color() const
{
	if (this->get_overlord() != nullptr) {
		return this->get_overlord()->get_game_data()->get_diplomatic_map_color();
	}

	return this->country->get_color();
}

}
