#include "metternich.h"

#include "map/province_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "game/game.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "util/assert_util.h"
#include "util/map_util.h"

namespace metternich {

province_game_data::province_game_data(const metternich::province *province) : province(province)
{
}

province_game_data::~province_game_data()
{
}

void province_game_data::set_owner(const country *country)
{
	if (country == this->get_owner()) {
		return;
	}

	const metternich::country *old_owner = this->owner;
	if (old_owner != nullptr) {
		old_owner->get_game_data()->remove_province(this->province);
	}

	this->owner = country;

	if (this->owner != nullptr) {
		this->owner->get_game_data()->add_province(this->province);
	}

	if (game::get()->is_running()) {
		for (const QPoint &tile_pos : this->get_border_tiles()) {
			map::get()->calculate_tile_country_border_directions(tile_pos);
		}

		map::get()->update_minimap_rect(this->get_territory_rect());

		emit owner_changed();

		if (old_owner == nullptr || this->owner == nullptr || old_owner->get_culture() != this->owner->get_culture()) {
			emit culture_changed();

			if (this->province->get_capital_settlement() != nullptr) {
				emit this->province->get_capital_settlement()->get_game_data()->culture_changed();
			}

			for (const QPoint &tile_pos : this->tiles) {
				const tile *tile = map::get()->get_tile(tile_pos);
				if (tile->get_site() != nullptr && tile->get_site() != this->province->get_capital_settlement()) {
					emit tile->get_site()->get_game_data()->culture_changed();
				}
			}
		}
	}
}

const culture *province_game_data::get_culture() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_culture();
	}

	return nullptr;
}

const std::string &province_game_data::get_current_cultural_name() const
{
	return this->province->get_cultural_name(this->get_culture());
}

void province_game_data::add_tile(const QPoint &tile_pos)
{
	this->tiles.push_back(tile_pos);

	const tile *tile = map::get()->get_tile(tile_pos);
	if (tile->get_resource() != nullptr) {
		this->resource_tiles.push_back(tile_pos);
		++this->resource_counts[tile->get_resource()];
	}
}

void province_game_data::add_border_tile(const QPoint &tile_pos)
{
	this->border_tiles.push_back(tile_pos);

	if (this->get_territory_rect().isNull()) {
		this->territory_rect = QRect(tile_pos, QSize(1, 1));
	} else {
		if (tile_pos.x() < this->territory_rect.x()) {
			this->territory_rect.setX(tile_pos.x());
		} else if (tile_pos.x() > this->territory_rect.right()) {
			this->territory_rect.setRight(tile_pos.x());
		}
		if (tile_pos.y() < this->territory_rect.y()) {
			this->territory_rect.setY(tile_pos.y());
		} else if (tile_pos.y() > this->territory_rect.bottom()) {
			this->territory_rect.setBottom(tile_pos.y());
		}
	}

	emit territory_changed();
}

void province_game_data::add_population_unit(qunique_ptr<population_unit> &&population_unit)
{
	this->change_population_type_count(population_unit->get_type(), 1);
	this->change_population(defines::get()->get_population_per_unit());

	this->population_units.push_back(std::move(population_unit));

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

qunique_ptr<population_unit> province_game_data::pop_population_unit(population_unit *population_unit)
{
	for (size_t i = 0; i < this->population_units.size();) {
		if (this->population_units[i].get() == population_unit) {
			this->change_population_type_count(population_unit->get_type(), -1);
			this->change_population(-defines::get()->get_population_per_unit());

			qunique_ptr<metternich::population_unit> population_unit_unique_ptr = std::move(this->population_units[i]);
			this->population_units.erase(this->population_units.begin() + i);

			if (game::get()->is_running()) {
				emit population_units_changed();
			}

			return population_unit_unique_ptr;
		} else {
			++i;
		}
	}

	assert_throw(false);

	return nullptr;
}

void province_game_data::create_population_unit(const population_type *type)
{
	auto population_unit = make_qunique<metternich::population_unit>(type, this->province);
	this->add_population_unit(std::move(population_unit));
}

void province_game_data::clear_population_units()
{
	this->population_units.clear();
	this->population_type_counts.clear();
	this->population = 0;
}

QVariantList province_game_data::get_population_type_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_type_counts());
}

void province_game_data::change_population_type_count(const population_type *type, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_type_counts[type] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_type_counts.erase(type);
	}

	if (game::get()->is_running()) {
		emit population_type_counts_changed();
	}
}

void province_game_data::change_population(const int change)
{
	this->population += change;

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_population(change);
	}
}

int province_game_data::get_score() const
{
	int score = province::base_score;

	for (const QPoint &tile_pos : this->resource_tiles) {
		const tile *tile = map::get()->get_tile(tile_pos);

		if (tile->get_improvement() != nullptr) {
			score += tile->get_improvement()->get_score();
		}
	}

	return score;
}

}
