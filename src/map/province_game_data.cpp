#include "metternich.h"

#include "map/province_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
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
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "script/scripted_province_modifier.h"
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
	this->reset_non_map_data();
}

province_game_data::~province_game_data()
{
}

void province_game_data::reset_non_map_data()
{
	this->set_owner(nullptr);
	this->free_food_consumption = province_game_data::base_free_food_consumption;
	this->clear_buildings();
	this->remove_capital_building_slots();
	this->score = province::base_score;
	this->clear_military_units();
	this->production_modifier = 0;
	this->commodity_production_modifiers.clear();
}

void province_game_data::on_map_created()
{
	this->calculate_territory_rect_center();
	this->initialize_building_slots();
}

void province_game_data::do_turn()
{
	this->do_production();

	for (const site *site : this->sites) {
		site->get_game_data()->do_turn();
	}

	this->decrement_scripted_modifiers();
}

void province_game_data::do_production()
{
	if (this->get_owner() == nullptr) {
		return;
	}

	const commodity_map<centesimal_int> output_per_commodity = this->get_commodity_outputs();
	country_game_data *owner_game_data = this->get_owner()->get_game_data();

	for (const auto &[commodity, output] : output_per_commodity) {
		const int output_int = output.to_int();

		if (output_int == 0) {
			continue;
		}

		owner_game_data->change_stored_commodity(commodity, output_int);
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
		if (this->is_capital()) {
			this->remove_capitol();
			this->remove_capital_building_slots();
		}

		old_owner->get_game_data()->remove_province(this->province);

		//remove the country modifier after removing the capitol, to ensure its effects won't be removed twice
		this->apply_country_modifier(old_owner, -1);
	}

	this->owner = country;

	if (this->owner != nullptr) {
		this->owner->get_game_data()->add_province(this->province);
		this->apply_country_modifier(this->owner, 1);

		if (this->is_capital()) {
			this->add_capital_building_slots();
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

void province_game_data::add_neighbor_province(const metternich::province *province)
{
	this->neighbor_provinces.push_back(province);

	if (province->is_sea() || province->is_bay()) {
		this->coastal = true;
	}
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

commodity_map<centesimal_int> province_game_data::get_commodity_outputs() const
{
	commodity_map<centesimal_int> output_per_commodity;

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
		const commodity_map<centesimal_int> building_output_per_commodity = building_slot->get_commodity_outputs();

		for (const auto &[commodity, output] : building_output_per_commodity) {
			output_per_commodity[commodity] += output;
		}
	}

	return output_per_commodity;
}

bool province_game_data::produces_commodity(const commodity *commodity) const
{
	const commodity_map<centesimal_int> output_per_commodity = this->get_commodity_outputs();
	const auto find_iterator = output_per_commodity.find(commodity);

	if (find_iterator != output_per_commodity.end() && find_iterator->second > 0) {
		return true;
	}

	return false;
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

void province_game_data::initialize_building_slots()
{
	//initialize building slots, placing them in random order
	std::vector<building_slot_type *> building_slot_types = building_slot_type::get_all();
	vector::shuffle(building_slot_types);

	const site *settlement = this->province->get_capital_settlement();

	for (const building_slot_type *building_slot_type : building_slot_types) {
		if (building_slot_type->is_capital()) {
			continue;
		}

		if (building_slot_type->is_coastal() && (settlement == nullptr || !map::get()->is_tile_coastal(settlement->get_game_data()->get_tile_pos()))) {
			continue;
		}

		if (building_slot_type->is_near_water() && (settlement == nullptr || !map::get()->is_tile_near_water(settlement->get_game_data()->get_tile_pos()))) {
			continue;
		}

		this->building_slots.push_back(make_qunique<building_slot>(building_slot_type, this->province));
		this->building_slot_map[building_slot_type] = this->building_slots.back().get();
	}
}

void province_game_data::add_capital_building_slots()
{
	assert_throw(this->is_capital());

	std::vector<building_slot_type *> building_slot_types = building_slot_type::get_all();
	vector::shuffle(building_slot_types);

	for (const building_slot_type *building_slot_type : building_slot_types) {
		if (!building_slot_type->is_capital()) {
			continue;
		}

		auto building_slot = make_qunique<metternich::building_slot>(building_slot_type, this->province);
		this->building_slot_map[building_slot_type] = building_slot.get();

		//insert the building slot at a random position
		const size_t slot_index = random::get()->generate(this->building_slots.size());
		this->building_slots.insert(this->building_slots.begin() + slot_index, std::move(building_slot));
	}
}

void province_game_data::remove_capital_building_slots()
{
	for (size_t i = 0; i < this->building_slots.size();) {
		const qunique_ptr<building_slot> &building_slot = this->building_slots.at(i);

		if (!building_slot->get_type()->is_capital()) {
			++i;
			continue;
		}

		this->building_slot_map.erase(building_slot->get_type());
		this->building_slots.erase(this->building_slots.begin() + i);
	}
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
		building_slot->calculate_base_commodity_outputs();
	}
}

void province_game_data::add_capitol()
{
	assert_throw(this->is_capital());
	assert_throw(this->get_culture() != nullptr);

	const building_class *capitol_building_class = defines::get()->get_capitol_building_class();
	const building_type *capitol_building_type = this->get_culture()->get_building_class_type(capitol_building_class);
	this->set_slot_building(capitol_building_class->get_slot_type(), capitol_building_type);

	//the capital gets a free warehouse if it doesn't have one
	bool has_warehouse = false;
	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		const building_type *building_type = building_slot->get_building();
		if (building_type == nullptr) {
			continue;
		}

		if (building_type->is_warehouse()) {
			has_warehouse = true;
			break;
		}
	}

	if (!has_warehouse) {
		for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
			const building_type *buildable_warehouse = nullptr;

			for (const building_type *building_type : building_slot->get_type()->get_building_types()) {
				if (!building_type->is_warehouse()) {
					continue;
				}

				if (building_type != this->get_culture()->get_building_class_type(building_type->get_building_class())) {
					continue;
				}

				if (building_type->get_required_technology() != nullptr) {
					//only a basic warehouse is free
					continue;
				}

				if (!building_slot->can_have_building(building_type)) {
					continue;
				}

				buildable_warehouse = building_type;
				break;
			}

			if (buildable_warehouse == nullptr) {
				continue;
			}

			building_slot->set_building(buildable_warehouse);
			break;
		}
	}
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

bool province_game_data::can_produce_commodity(const commodity *commodity) const
{
	for (const QPoint &tile_pos : this->resource_tiles) {
		const tile *tile = map::get()->get_tile(tile_pos);
		const metternich::commodity *tile_resource_commodity = tile->get_resource()->get_commodity();

		if (tile_resource_commodity == commodity) {
			return true;
		}
	}

	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		for (const building_type *building_type : building_slot->get_type()->get_building_types()) {
			if (building_type->get_output_commodity() != commodity) {
				continue;
			}

			if (building_type->get_required_building() != nullptr) {
				continue;
			}

			bool can_produce_inputs = true;
			for (const auto &[input_commodity, input_multiplier] : building_type->get_employment_type()->get_input_commodities()) {
				if (!this->can_produce_commodity(input_commodity)) {
					can_produce_inputs = false;
					break;
				}
			}

			if (can_produce_inputs) {
				return true;
			}
		}
	}

	return false;
}

void province_game_data::calculate_base_commodity_outputs()
{
	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		building_slot->calculate_base_commodity_outputs();
	}
}

void province_game_data::apply_country_modifier(const country *country, const int multiplier)
{
	for (const qunique_ptr<building_slot> &building_slot : this->building_slots) {
		if (building_slot->get_building() == nullptr) {
			continue;
		}

		building_slot->apply_country_modifier(country, multiplier);
	}
}

}
