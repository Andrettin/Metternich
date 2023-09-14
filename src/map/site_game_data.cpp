#include "metternich.h"

#include "map/site_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/settlement_building_slot.h"
#include "infrastructure/wonder.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/tile.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/modifier.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/map_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

site_game_data::site_game_data(const metternich::site *site) : site(site)
{
	if (site->is_settlement()) {
		this->initialize_building_slots();
	}
}

void site_game_data::reset_non_map_data()
{
	this->clear_buildings();
	this->clear_population_units();
	this->set_owner(nullptr);
	this->settlement_type = nullptr;
	this->commodity_outputs.clear();
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

void site_game_data::set_owner(const country *owner)
{
	if (owner == this->get_owner()) {
		return;
	}

	const country *old_owner = this->get_owner();

	this->owner = owner;

	if (this->site->is_settlement() && this->is_built()) {
		if (old_owner != nullptr) {
			old_owner->get_game_data()->change_settlement_count(-1);
		}

		if (owner != nullptr) {
			owner->get_game_data()->change_settlement_count(1);
		}

		this->check_building_conditions();
		this->check_free_buildings();
	}

	if (game::get()->is_running()) {
		emit owner_changed();
	}
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

void site_game_data::set_settlement_type(const metternich::settlement_type *settlement_type)
{
	assert_throw(this->site->is_settlement());

	if (settlement_type == this->get_settlement_type()) {
		return;
	}

	const metternich::settlement_type *old_settlement_type = this->get_settlement_type();

	this->settlement_type = settlement_type;

	if (this->get_owner() != nullptr) {
		if (old_settlement_type == nullptr && this->get_settlement_type() != nullptr) {
			this->get_province()->get_game_data()->change_settlement_count(1);
			this->get_owner()->get_game_data()->change_settlement_count(1);
		} else if (old_settlement_type != nullptr && this->get_settlement_type() == nullptr) {
			this->get_province()->get_game_data()->change_settlement_count(-1);
			this->get_owner()->get_game_data()->change_settlement_count(-1);
		}
	}

	if (this->get_settlement_type() != nullptr) {
		this->check_building_conditions();
		this->check_free_buildings();
	}

	if (game::get()->is_running()) {
		emit settlement_type_changed();
		emit map::get()->tile_settlement_type_changed(this->get_tile_pos());
	}
}

bool site_game_data::is_built() const
{
	assert_throw(this->site->is_settlement());

	return this->get_settlement_type() != nullptr;
}

const improvement *site_game_data::get_improvement() const
{
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_improvement();
	}

	return nullptr;
}

QVariantList site_game_data::get_building_slots_qvariant_list() const
{
	std::vector<const settlement_building_slot *> available_building_slots;

	for (const qunique_ptr<settlement_building_slot> &building_slot : this->building_slots) {
		if (!building_slot->is_available()) {
			continue;
		}

		available_building_slots.push_back(building_slot.get());
	}

	return container::to_qvariant_list(available_building_slots);
}

void site_game_data::initialize_building_slots()
{
	assert_throw(this->site->is_settlement());

	//initialize building slots, placing them in random order
	std::vector<building_slot_type *> building_slot_types = building_slot_type::get_all();
	vector::shuffle(building_slot_types);

	for (const building_slot_type *building_slot_type : building_slot_types) {
		this->building_slots.push_back(make_qunique<settlement_building_slot>(building_slot_type, this->site));
		this->building_slot_map[building_slot_type] = this->building_slots.back().get();
	}
}

const building_type *site_game_data::get_slot_building(const building_slot_type *slot_type) const
{
	const auto find_iterator = this->building_slot_map.find(slot_type);
	if (find_iterator != this->building_slot_map.end()) {
		return find_iterator->second->get_building();
	}

	assert_throw(false);

	return nullptr;
}

void site_game_data::set_slot_building(const building_slot_type *slot_type, const building_type *building)
{
	if (building != nullptr) {
		assert_throw(building->get_slot_type() == slot_type);
	}

	const auto find_iterator = this->building_slot_map.find(slot_type);
	if (find_iterator != this->building_slot_map.end()) {
		find_iterator->second->set_building(building);
		return;
	}

	assert_throw(false);
}

bool site_game_data::has_building(const building_type *building) const
{
	return this->get_slot_building(building->get_slot_type()) == building;
}

bool site_game_data::has_building_or_better(const building_type *building) const
{
	if (this->has_building(building)) {
		return true;
	}

	for (const building_type *requiring_building : building->get_requiring_buildings()) {
		if (this->has_building_or_better(requiring_building)) {
			return true;
		}
	}

	return false;
}

void site_game_data::clear_buildings()
{
	for (const qunique_ptr<settlement_building_slot> &building_slot : this->building_slots) {
		building_slot->set_wonder(nullptr);
		building_slot->set_building(nullptr);
	}
}

void site_game_data::check_building_conditions()
{
	assert_throw(this->site->is_settlement());

	for (const qunique_ptr<settlement_building_slot> &building_slot : this->building_slots) {
		const building_type *building = building_slot->get_building();

		if (building == nullptr) {
			continue;
		}

		//if the building fails its conditions, try to replace it with one of its required buildings, if valid
		while (building != nullptr) {
			if (building_slot->can_have_building(building)) {
				break;
			}

			building = building->get_required_building();
		}

		if (building != building_slot->get_building()) {
			building_slot->set_building(building);
		}
	}
}

void site_game_data::check_free_buildings()
{
	assert_throw(this->site->is_settlement());

	const country *owner = this->get_owner();
	if (owner == nullptr) {
		return;
	}

	const country_game_data *owner_game_data = owner->get_game_data();

	bool changed = false;

	for (const auto &[building_class, count] : owner_game_data->get_free_building_class_counts()) {
		assert_throw(count > 0);

		const building_type *building = this->get_culture()->get_building_class_type(building_class);

		if (building == nullptr) {
			continue;
		}

		if (this->check_free_building(building)) {
			changed = true;
		}
	}

	if (this->is_capital()) {
		for (const building_type *building : building_type::get_all()) {
			if (!building->is_free_in_capital()) {
				continue;
			}

			if (building != this->get_culture()->get_building_class_type(building->get_building_class())) {
				continue;
			}

			if (this->check_free_building(building)) {
				changed = true;
			}
		}
	}

	if (changed) {
		//check free buildings again, as the addition of a free building might have caused the requirements of others to be fulfilled
		this->check_free_buildings();
	}
}

bool site_game_data::check_free_building(const building_type *building)
{
	if (this->has_building_or_better(building)) {
		return false;
	}

	settlement_building_slot *building_slot = this->get_building_slot(building->get_slot_type());

	if (building_slot == nullptr) {
		return false;
	}

	if (!building_slot->can_gain_building(building)) {
		return false;
	}

	if (building->get_required_building() != nullptr && building_slot->get_building() != building->get_required_building()) {
		return false;
	}

	building_slot->set_building(building);
	return true;
}

void site_game_data::on_building_gained(const building_type *building, const int multiplier)
{
	assert_throw(building != nullptr);
	assert_throw(multiplier != 0);

	this->get_province()->get_game_data()->change_score(building->get_score() * multiplier);

	if (this->get_owner() != nullptr) {
		country_game_data *country_game_data = this->get_owner()->get_game_data();
		country_game_data->change_settlement_building_count(building, multiplier);
	}

	if (building->get_province_modifier() != nullptr) {
		building->get_province_modifier()->apply(this->get_province(), multiplier);
	}
}

void site_game_data::on_wonder_gained(const wonder *wonder, const int multiplier)
{
	assert_throw(wonder != nullptr);
	assert_throw(multiplier != 0);

	this->get_province()->get_game_data()->change_score(wonder->get_score() * multiplier);

	if (this->get_owner() != nullptr && wonder->get_country_modifier() != nullptr) {
		wonder->get_country_modifier()->apply(this->get_owner(), multiplier);
	}

	if (wonder->get_province_modifier() != nullptr) {
		wonder->get_province_modifier()->apply(this->get_province(), multiplier);
	}
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
