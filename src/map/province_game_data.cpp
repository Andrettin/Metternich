#include "metternich.h"

#include "map/province_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_turn_data.h"
#include "country/religion.h"
#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "economy/commodity_container.h"
#include "economy/production_type.h"
#include "economy/resource.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "game/province_event.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/settlement_building_slot.h"
#include "infrastructure/settlement_type.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_map_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_type.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_unit.h"
#include "population/profession.h"
#include "script/context.h"
#include "script/modifier.h"
#include "script/scripted_province_modifier.h"
#include "ui/icon.h"
#include "ui/icon_container.h"
#include "unit/army.h"
#include "unit/military_unit.h"
#include "unit/military_unit_category.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/image_util.h"
#include "util/map_util.h"
#include "util/point_util.h"

#include "xbrz.h"

namespace metternich {

province_game_data::province_game_data(const metternich::province *province)
	: province(province)
{
	this->population = make_qunique<metternich::population>();
	connect(this->get_population(), &population::main_culture_changed, this, &province_game_data::on_population_main_culture_changed);
	connect(this->get_population(), &population::main_religion_changed, this, &province_game_data::on_population_main_religion_changed);
}

province_game_data::~province_game_data()
{
}

void province_game_data::do_turn()
{
	for (const site *site : this->get_sites()) {
		site->get_game_data()->do_turn();
	}

	this->check_employment();

	this->decrement_scripted_modifiers();
}

void province_game_data::do_events()
{
	const bool is_last_turn_of_year = game::get()->get_year() != game::get()->get_next_date().year();

	if (is_last_turn_of_year) {
		province_event::check_events_for_scope(this->province, event_trigger::yearly_pulse);
	}

	province_event::check_events_for_scope(this->province, event_trigger::quarterly_pulse);
}

void province_game_data::do_ai_turn()
{
	//visit ruins (if any) with military units of this province's owner
	if (this->get_owner() != nullptr && this->has_country_military_unit(this->get_owner())) {
		for (const site *site : this->get_sites()) {
			site_game_data *site_game_data = site->get_game_data();
			if (!site_game_data->can_be_visited()) {
				continue;
			}

			std::vector<military_unit *> military_units = this->get_military_units();

			std::erase_if(military_units, [this](const military_unit *military_unit) {
				if (military_unit->get_country() != this->get_owner()) {
					return true;
				}

				if (military_unit->is_moving()) {
					return true;
				}

				return false;
			});

			if (!military_units.empty()) {
				auto army = make_qunique<metternich::army>(military_units, site);
				this->get_owner()->get_game_data()->add_army(std::move(army));
			}
			break;
		}
	}
}

bool province_game_data::is_on_map() const
{
	return this->province->get_map_data()->is_on_map();
}

void province_game_data::set_owner(const country *country)
{
	if (country == this->get_owner()) {
		return;
	}

	const metternich::country *old_owner = this->owner;

	this->owner = country;

	for (const site *site : this->get_sites()) {
		if (site->get_game_data()->get_owner() == old_owner) {
			site->get_game_data()->set_owner(country);
		}
	}

	if (old_owner != nullptr) {
		old_owner->get_game_data()->remove_province(this->province);
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->add_province(this->province);

		if (this->get_population()->get_main_culture() == nullptr) {
			this->set_culture(this->get_owner()->get_culture());
		}

		if (this->get_population()->get_main_religion() == nullptr) {
			this->set_religion(this->get_owner()->get_game_data()->get_religion());
		}
	} else {
		//remove population if this province becomes unowned
		for (population_unit *population_unit : this->population_units) {
			population_unit->get_settlement()->get_game_data()->pop_population_unit(population_unit);
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

	return this->get_owner()->get_game_data()->get_capital_province() == this->province;
}

void province_game_data::set_culture(const metternich::culture *culture)
{
	if (culture == this->get_culture()) {
		return;
	}

	this->culture = culture;

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::cultural);
		}
	}

	emit culture_changed();

	for (const site *site : this->get_sites()) {
		if (!site->is_settlement()) {
			site->get_game_data()->set_culture(culture);
		}
	}

	for (const site *settlement : this->get_settlement_sites()) {
		if (settlement->get_game_data()->get_population()->get_main_culture() == nullptr) {
			settlement->get_game_data()->set_culture(this->get_culture());
		}
	}
}

void province_game_data::on_population_main_culture_changed(const metternich::culture *culture)
{
	if (culture != nullptr) {
		this->set_culture(culture);
	} else if (this->get_owner() != nullptr) {
		this->set_culture(this->get_owner()->get_culture());
	} else {
		this->set_culture(nullptr);
	}
}

void province_game_data::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->religion = religion;

	for (const site *settlement : this->get_settlement_sites()) {
		if (settlement->get_game_data()->get_population()->get_main_religion() == nullptr) {
			settlement->get_game_data()->set_religion(this->get_religion());
		}
	}

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::religious);
		}
	}

	emit religion_changed();
}

void province_game_data::on_population_main_religion_changed(const metternich::religion *religion)
{
	if (religion != nullptr) {
		this->set_religion(religion);
	} else if (this->get_owner() != nullptr) {
		this->set_religion(this->get_owner()->get_game_data()->get_religion());
	} else {
		this->set_religion(nullptr);
	}
}

const std::string &province_game_data::get_current_cultural_name() const
{
	return this->province->get_cultural_name(this->get_culture());
}

bool province_game_data::is_coastal() const
{
	return this->province->get_map_data()->is_coastal();
}

bool province_game_data::is_near_water() const
{
	return this->province->get_map_data()->is_near_water();
}

const QRect &province_game_data::get_territory_rect() const
{
	return this->province->get_map_data()->get_territory_rect();
}

const QPoint &province_game_data::get_territory_rect_center() const
{
	return this->province->get_map_data()->get_territory_rect_center();
}

const std::vector<const metternich::province *> &province_game_data::get_neighbor_provinces() const
{
	return this->province->get_map_data()->get_neighbor_provinces();
}

bool province_game_data::is_country_border_province() const
{
	for (const metternich::province *neighbor_province : this->get_neighbor_provinces()) {
		const province_game_data *neighbor_province_game_data = neighbor_province->get_game_data();
		if (neighbor_province_game_data->get_owner() != this->get_owner()) {
			return true;
		}
	}

	return false;
}

const QPoint &province_game_data::get_center_tile_pos() const
{
	return this->province->get_map_data()->get_center_tile_pos();
}

const std::vector<QPoint> &province_game_data::get_border_tiles() const
{
	return this->province->get_map_data()->get_border_tiles();
}

const std::vector<QPoint> &province_game_data::get_resource_tiles() const
{
	return this->province->get_map_data()->get_resource_tiles();
}

const std::vector<const site *> &province_game_data::get_sites() const
{
	return this->province->get_map_data()->get_sites();
}

const std::vector<const site *> &province_game_data::get_settlement_sites() const
{
	return this->province->get_map_data()->get_settlement_sites();
}

bool province_game_data::produces_commodity(const commodity *commodity) const
{
	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);

		if (tile->produces_commodity(commodity)) {
			return true;
		}
	}

	return false;
}

const resource_map<int> &province_game_data::get_resource_counts() const
{
	return this->province->get_map_data()->get_resource_counts();
}

const terrain_type_map<int> &province_game_data::get_tile_terrain_counts() const
{
	return this->province->get_map_data()->get_tile_terrain_counts();
}

QVariantList province_game_data::get_scripted_modifiers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_scripted_modifiers());
}

bool province_game_data::has_scripted_modifier(const scripted_province_modifier *modifier) const
{
	return this->get_scripted_modifiers().contains(modifier);
}

void province_game_data::add_scripted_modifier(const scripted_province_modifier *modifier, const int duration)
{
	const read_only_context ctx(this->province);

	this->scripted_modifiers[modifier] = std::max(this->scripted_modifiers[modifier], duration);

	if (modifier->get_modifier() != nullptr) {
		this->apply_modifier(modifier->get_modifier());
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void province_game_data::remove_scripted_modifier(const scripted_province_modifier *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		this->remove_modifier(modifier->get_modifier());
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void province_game_data::decrement_scripted_modifiers()
{
	std::vector<const scripted_province_modifier *> modifiers_to_remove;
	for (auto &[modifier, duration] : this->scripted_modifiers) {
		--duration;

		if (duration == 0) {
			modifiers_to_remove.push_back(modifier);
		}
	}

	for (const scripted_province_modifier *modifier : modifiers_to_remove) {
		this->remove_scripted_modifier(modifier);
	}
}

void province_game_data::apply_modifier(const modifier<const metternich::province> *modifier, const int multiplier)
{
	assert_throw(modifier != nullptr);

	modifier->apply(this->province, multiplier);
}

void province_game_data::add_population_unit(population_unit *population_unit)
{
	this->population_units.push_back(population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void province_game_data::remove_population_unit(population_unit *population_unit)
{
	std::erase(this->population_units, population_unit);

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

void province_game_data::clear_population_units()
{
	this->population_units.clear();
}

QVariantList province_game_data::get_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_military_units());
}

void province_game_data::add_military_unit(military_unit *military_unit)
{
	this->military_units.push_back(military_unit);

	if (!military_unit->is_moving()) {
		this->change_military_unit_category_count(military_unit->get_category(), 1);
	}

	if (game::get()->is_running()) {
		emit military_units_changed();
	}
}

void province_game_data::remove_military_unit(military_unit *military_unit)
{
	std::erase(this->military_units, military_unit);

	if (!military_unit->is_moving()) {
		this->change_military_unit_category_count(military_unit->get_category(), -1);
	}

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
		if (military_unit->get_country() == country) {
			return true;
		}
	}

	return false;
}

QVariantList province_game_data::get_country_military_unit_category_counts(metternich::country *country) const
{
	std::map<military_unit_category, int> counts;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_country() == country && !military_unit->is_moving()) {
			++counts[military_unit->get_category()];
		}
	}

	return archimedes::map::to_qvariant_list(counts);
}

int province_game_data::get_country_military_unit_category_count(const metternich::military_unit_category category, metternich::country *country) const
{
	int count = 0;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_category() == category && military_unit->get_country() == country && !military_unit->is_moving()) {
			++count;
		}
	}

	return count;
}

const icon *province_game_data::get_military_unit_icon() const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (!military_unit->is_moving()) {
			++icon_counts[military_unit->get_icon()];
		}
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

	return best_icon;
}

const icon *province_game_data::get_military_unit_category_icon(const military_unit_category category) const
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

	return best_icon;
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

const icon *province_game_data::get_country_military_unit_icon(metternich::country *country) const
{
	icon_map<int> icon_counts;

	for (const military_unit *military_unit : this->get_military_units()) {
		if (military_unit->get_country() == country && !military_unit->is_moving()) {
			++icon_counts[military_unit->get_icon()];
		}
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

	return best_icon;
}

QVariantList province_game_data::get_entering_armies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_entering_armies());
}

void province_game_data::calculate_site_commodity_outputs()
{
	for (const site *site : this->get_sites()) {
		if (site->get_type() != site_type::settlement && site->get_type() != site_type::resource) {
			continue;
		}

		site->get_game_data()->calculate_commodity_outputs();
	}
}

void province_game_data::calculate_site_commodity_output(const commodity *commodity)
{
	for (const site *site : this->get_sites()) {
		if (site->get_type() != site_type::settlement && site->get_type() != site_type::resource) {
			continue;
		}

		if (!site->get_game_data()->produces_commodity(commodity) && !site->get_game_data()->get_base_commodity_outputs().contains(commodity)) {
			continue;
		}

		site->get_game_data()->calculate_commodity_outputs();
	}
}

void province_game_data::change_local_commodity_output(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int &output = (this->local_commodity_outputs[commodity] += change);

	if (output == 0) {
		this->local_commodity_outputs.erase(commodity);
	}

	if (commodity->is_health()) {
		this->province->get_provincial_capital()->get_game_data()->change_health(change);
	}
}

void province_game_data::change_improved_resource_commodity_bonus(const resource *resource, const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->improved_resource_commodity_bonuses[resource][commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->improved_resource_commodity_bonuses[resource].erase(commodity);

		if (this->improved_resource_commodity_bonuses[resource].empty()) {
			this->improved_resource_commodity_bonuses.erase(resource);
		}
	}

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		tile *tile = map::get()->get_tile(tile_pos);
		if (tile->get_resource() != resource) {
			continue;
		}

		assert_throw(tile->get_site() != nullptr);

		if (tile->get_site()->get_game_data()->get_resource_improvement() == nullptr) {
			continue;
		}

		tile->get_site()->get_game_data()->change_base_commodity_output(commodity, centesimal_int(change));
	}
}

void province_game_data::set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
{
	const int old_value = this->get_commodity_bonus_for_tile_threshold(commodity, threshold);

	if (value == old_value) {
		return;
	}

	if (value == 0) {
		this->commodity_bonuses_for_tile_thresholds[commodity].erase(threshold);

		if (this->commodity_bonuses_for_tile_thresholds[commodity].empty()) {
			this->commodity_bonuses_for_tile_thresholds.erase(commodity);
		}
	} else {
		this->commodity_bonuses_for_tile_thresholds[commodity][threshold] = value;
	}

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		tile *tile = map::get()->get_tile(tile_pos);
		if (!tile->produces_commodity(commodity)) {
			continue;
		}

		tile->calculate_commodity_outputs();
	}
}

bool province_game_data::can_produce_commodity(const commodity *commodity) const
{
	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const metternich::commodity *tile_resource_commodity = tile->get_resource()->get_commodity();

		if (tile_resource_commodity == commodity) {
			return true;
		}
	}

	return false;
}

std::vector<employment_location *> province_game_data::get_employment_locations() const
{
	std::vector<employment_location *> employment_locations;

	for (const QPoint &tile_pos : this->get_resource_tiles()) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const site *resource_site = tile->get_site();
		site_game_data *resource_site_game_data = resource_site->get_game_data();

		if (resource_site_game_data->get_employment_capacity() > 0) {
			employment_locations.push_back(resource_site_game_data);
		}
	}

	const site_game_data *provincial_capital_game_data = this->province->get_provincial_capital()->get_game_data();

	for (const qunique_ptr<settlement_building_slot> &building_slot : provincial_capital_game_data->get_building_slots()) {
		if (building_slot->get_employment_capacity() > 0) {
			employment_locations.push_back(building_slot.get());
		}
	}

	return employment_locations;
}

void province_game_data::check_employment()
{
	const std::vector<employment_location *> employment_locations = this->get_employment_locations();

	std::vector<population_unit *> unemployed_population_units;

	for (population_unit *population_unit : this->population_units) {
		if (population_unit->is_unemployed()) {
			unemployed_population_units.push_back(population_unit);
		}
	}

	std::vector<employment_location *> food_employment_locations = employment_locations;
	std::erase_if(food_employment_locations, [this](const employment_location *employment_location) {
		return !employment_location->get_employment_profession()->get_output_commodity()->is_food();
	});

	std::vector<employment_location *> non_food_employment_locations = employment_locations;
	std::erase_if(non_food_employment_locations, [this](const employment_location *employment_location) {
		return employment_location->get_employment_profession()->get_output_commodity()->is_food();
	});

	this->check_available_employment(food_employment_locations, unemployed_population_units);
	this->check_available_employment(non_food_employment_locations, unemployed_population_units);
}

void province_game_data::check_available_employment(const std::vector<employment_location *> &employment_locations, std::vector<population_unit *> &unemployed_population_units)
{
	for (employment_location *employment_location : employment_locations) {
		int available_employment_capacity = employment_location->get_available_employment_capacity();
		assert_throw(available_employment_capacity >= 0);
		if (available_employment_capacity == 0) {
			continue;
		}

		const profession *profession = employment_location->get_employment_profession();
		assert_throw(profession != nullptr);

		const commodity *output_commodity = profession->get_output_commodity();

		std::map<centesimal_int, std::vector<population_unit *>, std::greater<centesimal_int>> unemployed_population_units_by_output;
		for (population_unit *population_unit : unemployed_population_units) {
			if (!profession->can_employ(population_unit->get_type())) {
				continue;
			}

			unemployed_population_units_by_output[employment_location->get_employee_commodity_outputs(population_unit->get_type())[output_commodity]].push_back(population_unit);
		}

		for (const auto &[output, output_population_units] : unemployed_population_units_by_output) {
			for (population_unit *population_unit : output_population_units) {
				population_unit->set_employment_location(employment_location);
				--available_employment_capacity;
				std::erase(unemployed_population_units, population_unit);

				if (available_employment_capacity == 0) {
					break;
				}
			}

			if (available_employment_capacity == 0) {
				break;
			}
		}

		if (unemployed_population_units.empty()) {
			break;
		}
	}
}

}
