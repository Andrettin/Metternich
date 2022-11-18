#include "metternich.h"

#include "map/province_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/commodity_container.h"
#include "economy/employment_type.h"
#include "economy/resource.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "map/map.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/map_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

province_game_data::province_game_data(const metternich::province *province)
	: province(province), free_food_consumption(province_game_data::base_free_food_consumption)
{
	//initialize building slots, placing them in random order
	std::vector<building_slot_type *> building_slot_types = building_slot_type::get_all();
	vector::shuffle(building_slot_types);

	for (const building_slot_type *building_slot_type : building_slot_types) {
		this->building_slots.push_back(make_qunique<building_slot>(building_slot_type, this->province));
		this->building_slot_map[building_slot_type] = this->building_slots.back().get();
	}
}

province_game_data::~province_game_data()
{
}

void province_game_data::do_turn()
{
	this->assign_workers();
	this->do_production();

	while (this->get_population_growth() >= defines::get()->get_population_growth_threshold()) {
		this->grow_population();
	}

	while (this->get_population_growth() < 0) {
		//starvation
		this->decrease_population();
	}
}

void province_game_data::do_production()
{
	commodity_map<centesimal_int> output_per_commodity;

	for (const QPoint &tile_pos : this->resource_tiles) {
		const tile *tile = map::get()->get_tile(tile_pos);

		if (tile->get_improvement() == nullptr) {
			continue;
		}

		if (tile->get_improvement()->get_employment_type() == nullptr) {
			continue;
		}

		const employment_type *employment_type = tile->get_improvement()->get_employment_type();

		for (const population_unit *employee : tile->get_employees()) {
			output_per_commodity[employment_type->get_output_commodity()] += employee->get_employment_output(employment_type) * tile->get_improvement()->get_output_multiplier();
		}
	}

	//handle food
	if (this->get_population_unit_count() != 0) {
		centesimal_int food_output;
		for (const auto &[commodity, output] : output_per_commodity) {
			if (commodity->is_food()) {
				food_output += output;
			}
		}

		const int food_consumption = this->get_food_consumption() - this->get_free_food_consumption();
		const int net_food = food_output.to_int() - food_consumption;
		this->change_population_growth(net_food);
	} else {
		if (this->get_population_growth() != 0) {
			this->set_population_growth(0);
		}
	}
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

QVariantList province_game_data::get_building_slots_qvariant_list() const
{
	return container::to_qvariant_list(this->building_slots);
}

void province_game_data::set_slot_building(const building_slot_type *slot_type, const building_type *building)
{
	if (building != nullptr) {
		assert_throw(building->get_building_class()->get_slot_type() == slot_type);
	}

	const auto find_iterator = this->building_slot_map.find(slot_type);
	if (find_iterator != this->building_slot_map.end()) {
		find_iterator->second->set_building(building);
		return;
	}

	assert_throw(false);
}

void province_game_data::clear_buildings()
{
	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		building_slot->set_building(nullptr);
	}
}

void province_game_data::add_population_unit(qunique_ptr<population_unit> &&population_unit)
{
	this->change_population_type_count(population_unit->get_type(), 1);
	this->change_population_culture_count(population_unit->get_culture(), 1);
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
			if (population_unit->is_employed()) {
				this->unassign_worker(population_unit);
			}

			this->change_population_type_count(population_unit->get_type(), -1);
			this->change_population_culture_count(population_unit->get_culture(), -1);
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

void province_game_data::create_population_unit(const population_type *type, const culture *culture, const phenotype *phenotype)
{
	auto population_unit = make_qunique<metternich::population_unit>(type, culture, phenotype, this->province);
	this->add_population_unit(std::move(population_unit));
}

void province_game_data::clear_population_units()
{
	this->population_units.clear();
	this->population_type_counts.clear();
	this->population_culture_counts.clear();
	this->population = 0;
	this->population_growth = 0;
	this->free_food_consumption = province_game_data::base_free_food_consumption;
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

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_population_type_count(type, change);
		this->get_owner()->get_game_data()->change_score(change * population_unit::base_score);
	}

	if (game::get()->is_running()) {
		emit population_type_counts_changed();
	}
}

QVariantList province_game_data::get_population_culture_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_culture_counts());
}

void province_game_data::change_population_culture_count(const culture *culture, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_culture_counts[culture] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_culture_counts.erase(culture);
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_population_culture_count(culture, change);
	}

	if (game::get()->is_running()) {
		emit population_culture_counts_changed();
	}
}

void province_game_data::change_population(const int change)
{
	if (change == 0) {
		return;
	}

	this->population += change;

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_population(change);
	}

	if (game::get()->is_running()) {
		emit population_changed();
	}
}

void province_game_data::set_population_growth(const int growth)
{
	if (growth == this->get_population_growth()) {
		return;
	}

	const int change = growth - this->get_population_growth();

	this->population_growth = growth;

	this->change_population(change * defines::get()->get_population_per_unit() / defines::get()->get_population_growth_threshold());

	if (game::get()->is_running()) {
		emit population_growth_changed();
	}
}

void province_game_data::grow_population()
{
	if (this->population_units.empty()) {
		throw std::runtime_error("Tried to grow population in a province which has no pre-existing population.");
	}

	const qunique_ptr<population_unit> &population_unit = vector::get_random(this->population_units);
	const culture *culture = population_unit->get_culture();
	const phenotype *phenotype = population_unit->get_phenotype();
	const population_type *population_type = culture->get_population_class_type(defines::get()->get_default_population_class());

	this->create_population_unit(population_type, culture, phenotype);
	this->assign_worker(this->population_units.back().get());

	this->change_population_growth(-defines::get()->get_population_growth_threshold());
}

void province_game_data::decrease_population()
{
	this->change_population_growth(defines::get()->get_population_growth_threshold());

	if (this->population_units.empty()) {
		return;
	}

	population_unit *best_population_unit = nullptr;

	for (auto it = this->population_units.rbegin(); it != this->population_units.rend(); ++it) {
		population_unit *population_unit = it->get();

		if (
			best_population_unit == nullptr
			|| (best_population_unit->produces_food() && !population_unit->produces_food())
			|| (best_population_unit->produces_food() == population_unit->produces_food() && best_population_unit->get_employment_output() < population_unit->get_employment_output())
		) {
			best_population_unit = population_unit;
		}
	}

	assert_throw(best_population_unit != nullptr);

	this->pop_population_unit(best_population_unit);
}

QObject *province_game_data::get_population_type_small_icon(population_type *type) const
{
	icon_map<int> icon_counts;

	for (const auto &population_unit : this->population_units) {
		if (population_unit->get_type() != type) {
			continue;
		}

		++icon_counts[population_unit->get_small_icon()];
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	return const_cast<icon *>(best_icon);
}

void province_game_data::assign_workers()
{
	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		if (population_unit->is_employed()) {
			continue;
		}

		this->assign_worker(population_unit.get());
	}
}

void province_game_data::assign_worker(population_unit *population_unit)
{
	for (const QPoint &tile_pos : this->resource_tiles) {
		tile *tile = map::get()->get_tile(tile_pos);

		if (!tile->get_resource()->get_commodity()->is_food()) {
			//give priority to food-producing tiles
			continue;
		}

		const bool assigned = this->try_assign_worker_to_tile(population_unit, tile);
		if (assigned) {
			return;
		}
	}

	for (const QPoint &tile_pos : this->resource_tiles) {
		tile *tile = map::get()->get_tile(tile_pos);

		if (tile->get_resource()->get_commodity()->is_food()) {
			//already processed
			continue;
		}

		const bool assigned = this->try_assign_worker_to_tile(population_unit, tile);
		if (assigned) {
			return;
		}
	}
}

bool province_game_data::try_assign_worker_to_tile(population_unit *population_unit, tile *tile)
{
	if (tile->get_improvement() == nullptr) {
		return false;
	}

	if (tile->get_improvement()->get_employment_type() == nullptr) {
		return false;
	}

	if (tile->get_employee_count() >= tile->get_employment_capacity()) {
		return false;
	}

	if (!vector::contains(tile->get_improvement()->get_employment_type()->get_employees(), population_unit->get_type()->get_population_class())) {
		return false;
	}

	this->assign_worker_to_tile(population_unit, tile);
	return true;
}

void province_game_data::assign_worker_to_tile(population_unit *population_unit, tile *tile)
{
	tile->add_employee(population_unit);
	population_unit->set_employment_type(tile->get_improvement()->get_employment_type());

	if (tile->get_resource()->get_commodity()->is_food()) {
		//food-producing workers don't consume food
		this->free_food_consumption += 1;
	}
}

void province_game_data::unassign_worker(population_unit *population_unit)
{
	for (const QPoint &tile_pos : this->resource_tiles) {
		tile *tile = map::get()->get_tile(tile_pos);

		if (!vector::contains(tile->get_employees(), population_unit)) {
			continue;
		}

		tile->remove_employee(population_unit);
		population_unit->set_employment_type(nullptr);

		if (tile->get_resource()->get_commodity()->is_food()) {
			this->free_food_consumption -= 1;
		}
		break;
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

	for (const auto &[population_type, count] : this->get_population_type_counts()) {
		score += population_unit::base_score * count;
	}

	return score;
}

}
