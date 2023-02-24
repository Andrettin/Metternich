#include "metternich.h"

#include "map/province_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/ideology.h"
#include "country/religion.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "economy/commodity_container.h"
#include "economy/employment_type.h"
#include "economy/resource.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/province_event.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "population/phenotype.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "unit/military_unit.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/image_util.h"
#include "util/map_util.h"
#include "util/point_util.h"
#include "util/thread_pool.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

#include "xbrz.h"

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

	this->reset_non_map_data();
}

province_game_data::~province_game_data()
{
}

void province_game_data::reset_non_map_data()
{
	this->set_owner(nullptr);
	this->clear_population_units();
	this->clear_buildings();
	this->score = province::base_score;
	this->clear_military_units();
}

void province_game_data::do_turn()
{
	this->assign_workers();
	this->do_production();
	this->do_cultural_change();

	for (const site *site : this->sites) {
		site->get_game_data()->do_turn();
	}
}

void province_game_data::do_production()
{
	if (this->get_owner() == nullptr) {
		return;
	}

	commodity_map<centesimal_int> output_per_commodity;
	country_game_data *owner_game_data = this->get_owner()->get_game_data();

	for (const QPoint &tile_pos : this->resource_tiles) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const improvement *improvement = tile->get_improvement();

		if (improvement == nullptr) {
			continue;
		}

		if (improvement->get_employment_type() == nullptr) {
			continue;
		}

		const employment_type *employment_type = improvement->get_employment_type();
		const commodity *output_commodity = employment_type->get_output_commodity();
		centesimal_int output;

		for (const population_unit *employee : tile->get_employees()) {
			output += employee->get_employment_output(employment_type);
		}

		const centesimal_int output_multiplier = tile->get_output_multiplier();

		output_per_commodity[output_commodity] += output * output_multiplier;
	}

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		const building_type *building_type = building_slot->get_building();
		if (building_type == nullptr) {
			continue;
		}

		if (building_type->get_employment_type() == nullptr) {
			continue;
		}

		const employment_type *employment_type = building_type->get_employment_type();
		const commodity *output_commodity = employment_type->get_output_commodity();
		centesimal_int output;

		for (const population_unit *employee : building_slot->get_employees()) {
			output += employee->get_employment_output(employment_type);
		}

		const centesimal_int output_multiplier = building_slot->get_output_multiplier();

		output_per_commodity[output_commodity] += output * output_multiplier;
	}

	for (const auto &[commodity, output] : output_per_commodity) {
		owner_game_data->change_stored_commodity(commodity, output.to_int());
	}
}

void province_game_data::do_cultural_change()
{
	static constexpr int cultural_derivation_chance = 1;

	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		const metternich::culture *current_culture = population_unit->get_culture();

		std::vector<const metternich::culture *> potential_cultures;

		const read_only_context ctx(population_unit.get());

		for (const metternich::culture *culture : current_culture->get_derived_cultures()) {
			if (!this->province->has_core_country_of_culture(culture)) {
				continue;
			}

			if (culture->get_derivation_conditions() != nullptr && !culture->get_derivation_conditions()->check(population_unit.get(), ctx)) {
				continue;
			}

			potential_cultures.push_back(culture);
		}

		if (!potential_cultures.empty() && random::get()->generate(100) < cultural_derivation_chance) {
			const metternich::culture *new_culture = vector::get_random(potential_cultures);
			population_unit->set_culture(new_culture);
		}
	}
}

void province_game_data::do_events()
{
	const bool is_last_turn_of_year = (game::get()->get_date().date().month() + defines::get()->get_months_per_turn()) > 12;

	if (is_last_turn_of_year) {
		province_event::check_events_for_scope(this->province, event_trigger::yearly_pulse);
	}

	province_event::check_events_for_scope(this->province, event_trigger::quarterly_pulse);
}

void province_game_data::do_ai_turn()
{
	//visit ruins (if any) with military units of this province's owner
	if (this->get_owner() != nullptr && this->has_country_military_unit(this->get_owner())) {
		for (const site *site : this->sites) {
			site_game_data *site_game_data = site->get_game_data();
			if (site_game_data->get_improvement() == nullptr || !site_game_data->get_improvement()->is_ruins()) {
				continue;
			}

			const std::vector<military_unit *> military_units = this->get_military_units();
			for (military_unit *military_unit : military_units) {
				if (military_unit->get_owner() != this->get_owner()) {
					continue;
				}

				military_unit->visit_site(site);
			}
			break;
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

		if (this->is_capital()) {
			this->remove_capitol();
		}
	}

	this->owner = country;

	if (this->owner != nullptr) {
		this->owner->get_game_data()->add_province(this->province);

		if (this->get_population_unit_count() == 0) {
			this->calculate_culture();
			this->calculate_religion();
		}

		if (this->is_capital()) {
			this->add_capitol();
		}
	}

	if (game::get()->is_running()) {
		for (const QPoint &tile_pos : this->get_border_tiles()) {
			map::get()->calculate_tile_country_border_directions(tile_pos);
		}

		map::get()->update_minimap_rect(this->get_territory_rect());

		emit owner_changed();
	}
}

bool province_game_data::is_capital() const
{
	if (this->get_owner() == nullptr) {
		return false;
	}

	return this->get_owner()->get_capital_province() == this->province;
}

void province_game_data::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	this->culture = culture;

	if (culture != nullptr) {
		//update buildings for the new culture
		for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
			const building_type *building = building_slot->get_building();

			if (building == nullptr) {
				continue;
			}

			const building_type *new_building = culture->get_building_class_type(building->get_building_class());

			if (new_building != building) {
				building_slot->set_building(building);
			}
		}
	}

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			thread_pool::get()->co_spawn_sync([this]() -> boost::asio::awaitable<void> {
				co_await this->get_owner()->get_game_data()->create_diplomatic_map_mode_image(diplomatic_map_mode::cultural, {});
			});
		}
	}

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

void province_game_data::calculate_culture()
{
	if (this->get_population_unit_count() == 0) {
		if (this->get_owner() != nullptr) {
			this->set_culture(this->get_owner()->get_culture());
		} else {
			this->set_culture(nullptr);
		}

		return;
	}

	const metternich::culture *best_culture = nullptr;
	int best_count = 0;

	for (const auto &[culture, count] : this->get_population_culture_counts()) {
		if (count > best_count) {
			best_count = count;
			best_culture = culture;
		}
	}

	this->set_culture(best_culture);
}

void province_game_data::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->religion = religion;

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			thread_pool::get()->co_spawn_sync([this]() -> boost::asio::awaitable<void> {
				co_await this->get_owner()->get_game_data()->create_diplomatic_map_mode_image(diplomatic_map_mode::religious, {});
			});
		}
	}

	emit religion_changed();
}

void province_game_data::calculate_religion()
{
	if (this->get_population_unit_count() == 0) {
		if (this->get_owner() != nullptr) {
			this->set_religion(this->get_owner()->get_game_data()->get_religion());
		} else {
			this->set_religion(nullptr);
		}

		return;
	}

	const metternich::religion *best_religion = nullptr;
	int best_count = 0;

	for (const auto &[religion, count] : this->get_population_religion_counts()) {
		if (count > best_count) {
			best_count = count;
			best_religion = religion;
		}
	}

	this->set_religion(best_religion);
}

const std::string &province_game_data::get_current_cultural_name() const
{
	return this->province->get_cultural_name(this->get_culture());
}

void province_game_data::calculate_territory_rect_center()
{
	const int tile_count = static_cast<int>(this->get_tiles().size());

	assert_throw(tile_count > 0);

	QPoint sum(0, 0);

	for (const QPoint &tile_pos : this->get_tiles()) {
		sum += tile_pos;
	}

	this->territory_rect_center = QPoint(sum.x() / tile_count, sum.y() / tile_count);
}

void province_game_data::add_border_province(const metternich::province *province)
{
	this->border_provinces.push_back(province);

	if (province->is_sea() || province->is_bay()) {
		this->coastal = true;
	}
}

void province_game_data::add_tile(const QPoint &tile_pos)
{
	this->tiles.push_back(tile_pos);

	const tile *tile = map::get()->get_tile(tile_pos);

	if (tile->get_resource() != nullptr) {
		this->resource_tiles.push_back(tile_pos);
		++this->resource_counts[tile->get_resource()];
	}

	if (tile->get_terrain() != nullptr) {
		++this->tile_terrain_counts[tile->get_terrain()];
	}

	if (tile->get_site() != nullptr) {
		this->sites.push_back(tile->get_site());
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

void province_game_data::on_improvement_gained(const improvement *improvement, const int multiplier)
{
	assert_throw(improvement != nullptr);

	this->change_score(improvement->get_score() * multiplier);
}

QVariantList province_game_data::get_building_slots_qvariant_list() const
{
	return container::to_qvariant_list(this->building_slots);
}

const building_type *province_game_data::get_slot_building(const building_slot_type *slot_type) const
{
	const auto find_iterator = this->building_slot_map.find(slot_type);
	if (find_iterator != this->building_slot_map.end()) {
		return find_iterator->second->get_building();
	}

	assert_throw(false);

	return nullptr;
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
		building_slot->clear_employees();
		building_slot->set_building(nullptr);
	}
}

void province_game_data::add_capitol()
{
	assert_throw(this->is_capital());
	assert_throw(this->get_culture() != nullptr);

	const building_class *capitol_building_class = defines::get()->get_capitol_building_class();
	const building_type *capitol_building_type = this->get_culture()->get_building_class_type(capitol_building_class);
	this->set_slot_building(capitol_building_class->get_slot_type(), capitol_building_type);
}

void province_game_data::remove_capitol()
{
	const building_class *capitol_building_class = defines::get()->get_capitol_building_class();
	const building_slot_type *capitol_slot_type = capitol_building_class->get_slot_type();

	const building_type *current_slot_building = this->get_slot_building(capitol_slot_type);
	if (current_slot_building == nullptr || current_slot_building->get_building_class() != capitol_building_class) {
		return;
	}

	//remove the capitol and replace it with the next-best building
	const building_type *best_building_type = nullptr;

	if (this->get_culture() != nullptr) {
		for (const building_type *building_type : capitol_slot_type->get_building_types()) {
			if (this->get_culture()->get_building_class_type(building_type->get_building_class()) != building_type) {
				continue;
			}

			if (best_building_type == nullptr || best_building_type->get_score() < building_type->get_score()) {
				best_building_type = building_type;
			}
		}
	}

	this->set_slot_building(capitol_slot_type, best_building_type);
}

void province_game_data::on_building_gained(const building_type *building, const int multiplier)
{
	assert_throw(building != nullptr);

	this->change_score(building->get_score() * multiplier);
}

void province_game_data::add_population_unit(qunique_ptr<population_unit> &&population_unit)
{
	this->change_population_type_count(population_unit->get_type(), 1);
	this->change_population_culture_count(population_unit->get_culture(), 1);
	this->change_population_religion_count(population_unit->get_religion(), 1);
	this->change_population_phenotype_count(population_unit->get_phenotype(), 1);
	if (population_unit->get_ideology() != nullptr) {
		this->change_population_ideology_count(population_unit->get_ideology(), 1);
	}
	this->change_population(defines::get()->get_population_per_unit());
	this->change_total_consciousness(population_unit->get_consciousness());
	this->change_total_militancy(population_unit->get_militancy());

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->add_population_unit(population_unit.get());
	}

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

			qunique_ptr<metternich::population_unit> population_unit_unique_ptr = std::move(this->population_units[i]);
			this->population_units.erase(this->population_units.begin() + i);

			if (this->get_owner() != nullptr) {
				this->get_owner()->get_game_data()->remove_population_unit(population_unit);
			}

			this->change_population_type_count(population_unit->get_type(), -1);
			this->change_population_culture_count(population_unit->get_culture(), -1);
			this->change_population_religion_count(population_unit->get_religion(), -1);
			this->change_population_phenotype_count(population_unit->get_phenotype(), -1);
			if (population_unit->get_ideology() != nullptr) {
				this->change_population_ideology_count(population_unit->get_ideology(), -1);
			}
			this->change_population(-defines::get()->get_population_per_unit());
			this->change_total_consciousness(-population_unit->get_consciousness());
			this->change_total_militancy(-population_unit->get_militancy());

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

void province_game_data::create_population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const phenotype *phenotype)
{
	auto population_unit = make_qunique<metternich::population_unit>(type, culture, religion, phenotype, this->province);
	this->add_population_unit(std::move(population_unit));
}

void province_game_data::clear_population_units()
{
	this->population_units.clear();
	this->population_type_counts.clear();
	this->population_culture_counts.clear();
	this->population_religion_counts.clear();
	this->population_phenotype_counts.clear();
	this->population_ideology_counts.clear();
	this->population = 0;
	this->free_food_consumption = province_game_data::base_free_food_consumption;
	this->total_consciousness = centesimal_int(0);
	this->total_militancy = centesimal_int(0);
	this->home_civilian_units.clear();
	this->home_military_units.clear();
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
		this->change_score(change * population_unit::base_score);
	}

	if (game::get()->is_running()) {
		emit population_type_counts_changed();
	}
}

QVariantList province_game_data::get_population_culture_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_culture_counts());
}

void province_game_data::change_population_culture_count(const metternich::culture *culture, const int change)
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
		this->calculate_culture();

		emit population_culture_counts_changed();
	}
}

QVariantList province_game_data::get_population_religion_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_religion_counts());
}

void province_game_data::change_population_religion_count(const metternich::religion *religion, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_religion_counts[religion] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_religion_counts.erase(religion);
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_population_religion_count(religion, change);
	}

	if (game::get()->is_running()) {
		this->calculate_religion();

		emit population_religion_counts_changed();
	}
}

QVariantList province_game_data::get_population_phenotype_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_phenotype_counts());
}

void province_game_data::change_population_phenotype_count(const phenotype *phenotype, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->population_phenotype_counts[phenotype] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_phenotype_counts.erase(phenotype);
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_population_phenotype_count(phenotype, change);
	}

	if (game::get()->is_running()) {
		emit population_phenotype_counts_changed();
	}
}

QVariantList province_game_data::get_population_ideology_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_population_ideology_counts());
}

void province_game_data::change_population_ideology_count(const ideology *ideology, const int change)
{
	if (change == 0) {
		return;
	}

	assert_throw(ideology != nullptr);

	const int count = (this->population_ideology_counts[ideology] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->population_ideology_counts.erase(ideology);
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_population_ideology_count(ideology, change);
	}

	if (game::get()->is_running()) {
		emit population_ideology_counts_changed();
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

void province_game_data::grow_population()
{
	if (this->population_units.empty()) {
		throw std::runtime_error("Tried to grow population in a province which has no pre-existing population.");
	}

	assert_throw(this->get_owner() != nullptr);

	const qunique_ptr<population_unit> &population_unit = vector::get_random(this->population_units);
	const metternich::culture *culture = population_unit->get_culture();
	const metternich::religion *religion = population_unit->get_religion();
	const phenotype *phenotype = population_unit->get_phenotype();
	const population_type *population_type = culture->get_population_class_type(defines::get()->get_default_population_class());

	this->create_population_unit(population_type, culture, religion, phenotype);
	this->assign_worker(this->population_units.back().get());

	this->get_owner()->get_game_data()->change_population_growth(-defines::get()->get_population_growth_threshold());
}

void province_game_data::decrease_population()
{
	assert_throw(!this->population_units.empty());

	this->get_owner()->get_game_data()->change_population_growth(defines::get()->get_population_growth_threshold());

	this->pop_population_unit(this->choose_starvation_population_unit());
}

population_unit *province_game_data::choose_starvation_population_unit()
{
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
	return best_population_unit;
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

void province_game_data::reassign_workers()
{
	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		if (!population_unit->is_employed()) {
			continue;
		}

		this->unassign_worker(population_unit.get());
	}

	this->assign_workers();
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

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		const building_type *building_type = building_slot->get_building();
		if (building_type == nullptr) {
			continue;
		}

		const employment_type *employment_type = building_type->get_employment_type();
		if (employment_type == nullptr) {
			continue;
		}

		if (!employment_type->get_output_commodity()->is_food()) {
			//give priority to food-producing buildings
			continue;
		}

		const bool assigned = this->try_assign_worker_to_building(population_unit, building_slot.get());
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

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		const building_type *building_type = building_slot->get_building();
		if (building_type == nullptr) {
			continue;
		}

		const employment_type *employment_type = building_type->get_employment_type();
		if (employment_type == nullptr) {
			continue;
		}

		if (employment_type->get_output_commodity()->is_food()) {
			//already processed
			continue;
		}

		const bool assigned = this->try_assign_worker_to_building(population_unit, building_slot.get());
		if (assigned) {
			return;
		}
	}
}

bool province_game_data::try_assign_worker_to_tile(population_unit *population_unit, tile *tile)
{
	if (!this->can_tile_employ_worker(population_unit, tile)) {
		return false;
	}

	this->assign_worker_to_tile(population_unit, tile);
	return true;
}

void province_game_data::assign_worker_to_tile(population_unit *population_unit, tile *tile)
{
	tile->add_employee(population_unit);
	population_unit->set_employment_type(tile->get_improvement()->get_employment_type());

	//workers who work on tiles don't consume food
	this->free_food_consumption += 1;
}

bool province_game_data::try_assign_worker_to_building(population_unit *population_unit, building_slot *building_slot)
{
	if (!this->can_building_employ_worker(population_unit, building_slot)) {
		return false;
	}

	this->assign_worker_to_building(population_unit, building_slot);
	return true;
}

void province_game_data::assign_worker_to_building(population_unit *population_unit, building_slot *building_slot)
{
	building_slot->add_employee(population_unit);
	population_unit->set_employment_type(building_slot->get_building()->get_employment_type());
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

		this->free_food_consumption -= 1;
		return;
	}

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		if (!vector::contains(building_slot->get_employees(), population_unit)) {
			continue;
		}

		building_slot->remove_employee(population_unit);
		population_unit->set_employment_type(nullptr);
		return;
	}
}

bool province_game_data::can_tile_employ_worker(const population_unit *population_unit, const tile *tile) const
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

	return true;
}

bool province_game_data::can_building_employ_worker(const population_unit *population_unit, const building_slot *building_slot) const
{
	if (building_slot->get_building() == nullptr) {
		return false;
	}

	if (building_slot->get_building()->get_employment_type() == nullptr) {
		return false;
	}

	if (building_slot->get_employee_count() >= building_slot->get_employment_capacity()) {
		return false;
	}

	if (!vector::contains(building_slot->get_building()->get_employment_type()->get_employees(), population_unit->get_type()->get_population_class())) {
		return false;
	}

	return true;
}

bool province_game_data::has_employment_for_worker(const population_unit *population_unit) const
{
	for (const QPoint &tile_pos : this->resource_tiles) {
		const tile *tile = map::get()->get_tile(tile_pos);

		if (this->can_tile_employ_worker(population_unit, tile)) {
			return true;
		}
	}

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		if (this->can_building_employ_worker(population_unit, building_slot.get())) {
			return true;
		}
	}

	return false;
}

int province_game_data::get_consciousness() const
{
	if (this->get_population_unit_count() == 0) {
		return 0;
	}

	return (this->get_total_consciousness() / this->get_population_unit_count()).to_int();
}

void province_game_data::set_total_consciousness(const centesimal_int &consciousness)
{
	if (consciousness == this->get_total_consciousness()) {
		return;
	}

	this->total_consciousness = consciousness;

	if (game::get()->is_running()) {
		emit consciousness_changed();
	}
}

int province_game_data::get_militancy() const
{
	if (this->get_population_unit_count() == 0) {
		return 0;
	}

	return (this->get_total_militancy() / this->get_population_unit_count()).to_int();
}

void province_game_data::set_total_militancy(const centesimal_int &militancy)
{
	if (militancy == this->get_total_militancy()) {
		return;
	}

	this->total_militancy = militancy;

	if (game::get()->is_running()) {
		emit militancy_changed();
	}
}

void province_game_data::change_score(const int change)
{
	if (change == 0) {
		return;
	}

	this->score += change;

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_score(change);
	}
}

void province_game_data::add_military_unit(military_unit *military_unit)
{
	this->military_units.push_back(military_unit);

	this->change_military_unit_category_count(military_unit->get_category(), 1);

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void province_game_data::remove_military_unit(military_unit *military_unit)
{
	std::erase(this->military_units, military_unit);

	this->change_military_unit_category_count(military_unit->get_category(), -1);

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void province_game_data::clear_military_units()
{
	this->military_units.clear();
	this->military_unit_category_counts.clear();
}

QVariantList province_game_data::get_military_unit_category_counts_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->military_unit_category_counts);
}

void province_game_data::change_military_unit_category_count(const military_unit_category category, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->military_unit_category_counts[category] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->military_unit_category_counts.erase(category);
	}

	if (game::get()->is_running()) {
		emit military_unit_category_counts_changed();
	}
}

bool province_game_data::has_country_military_unit(const country *country) const
{
	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_owner() == country) {
			return true;
		}
	}

	return false;
}

int province_game_data::get_country_military_unit_category_count(const metternich::military_unit_category category, metternich::country *country) const
{
	int count = 0;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_category() == category && military_unit->get_owner() == country) {
			++count;
		}
	}

	return count;
}

QObject *province_game_data::get_military_unit_category_icon(const military_unit_category category) const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->military_units) {
		if (military_unit->get_category() != category) {
			continue;
		}

		++icon_counts[military_unit->get_icon()];
	}

	const icon *best_icon = nullptr;
	int best_icon_count = 0;
	for (const auto &[icon, count] : icon_counts) {
		if (count > best_icon_count) {
			best_icon = icon;
			best_icon_count = count;
		}
	}

	assert_throw(best_icon != nullptr);

	return const_cast<icon *>(best_icon);
}

QString province_game_data::get_military_unit_category_name(const military_unit_category category) const
{
	std::map<QString, int> name_counts;

	for (const military_unit *military_unit : this->military_units) {
		if (military_unit->get_category() != category) {
			continue;
		}

		++name_counts[military_unit->get_type()->get_name_qstring()];
	}

	QString best_name;
	int best_name_count = 0;
	for (const auto &[name, count] : name_counts) {
		if (count > best_name_count) {
			best_name = name;
			best_name_count = count;
		}
	}

	assert_throw(!best_name.isEmpty());

	return best_name;
}

}
