#include "metternich.h"

#include "map/site_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/country_turn_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/settlement_building_slot.h"
#include "infrastructure/settlement_type.h"
#include "infrastructure/wonder.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_map_data.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/modifier.h"
#include "script/scripted_site_modifier.h"
#include "unit/army.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

site_game_data::site_game_data(const metternich::site *site) : site(site)
{
	if (site->is_settlement()) {
		this->initialize_building_slots();
	}

	this->housing = defines::get()->get_base_housing();
	this->free_food_consumption = site_game_data::base_free_food_consumption;

	const resource *resource = site->get_map_data()->get_resource();
	if (resource == nullptr || resource->get_required_technology() != nullptr || resource->is_prospectable()) {
		this->set_resource_discovered(false);
	} else {
		this->set_resource_discovered(true);
	}

	this->population = make_qunique<metternich::population>();
	connect(this->get_population(), &population::type_count_changed, this, &site_game_data::on_population_type_count_changed);
	connect(this->get_population(), &population::main_culture_changed, this, &site_game_data::on_population_main_culture_changed);
	connect(this->get_population(), &population::main_religion_changed, this, &site_game_data::on_population_main_religion_changed);
	if (this->get_province() != nullptr) {
		this->get_population()->add_upper_population(this->get_province()->get_game_data()->get_population());
	}
}

void site_game_data::do_turn()
{
	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		population_unit->do_turn();
	}

	this->decrement_scripted_modifiers();
}

void site_game_data::do_consumption()
{
	std::vector<population_unit *> shuffled_population_units;
	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		shuffled_population_units.push_back(population_unit.get());
	}
	vector::shuffle(shuffled_population_units);

	std::vector<population_unit *> population_units;
	for (population_unit *population_unit : shuffled_population_units) {
		if (population_unit->is_consumption_fulfilled()) {
			population_units.push_back(population_unit);
		} else {
			population_units.insert(population_units.begin(), population_unit);
		}
	}

	for (const auto &[commodity, consumption] : this->local_commodity_consumptions) {
		assert_throw(commodity->is_local());
		assert_throw(!commodity->is_storable());

		const int effective_consumption = std::min(consumption.to_int(), this->is_provincial_capital() ? this->get_province()->get_game_data()->get_local_commodity_output(commodity).to_int() : this->get_commodity_output(commodity).to_int());

		centesimal_int remaining_consumption(consumption.to_int() - effective_consumption);
		if (remaining_consumption == 0) {
			continue;
		}

		//go through population units belonging to the settlement in random order, and cause the effects of them not being able to have their consumption fulfilled
		for (population_unit *population_unit : population_units) {
			const centesimal_int pop_consumption = population_unit->get_type()->get_commodity_consumption(commodity);
			if (pop_consumption == 0) {
				continue;
			}

			population_unit->set_consumption_fulfilled(false);
			remaining_consumption -= pop_consumption;

			if (remaining_consumption <= 0) {
				break;
			}
		}
	}
}

const QPoint &site_game_data::get_tile_pos() const
{
	return this->site->get_map_data()->get_tile_pos();
}

tile *site_game_data::get_tile() const
{
	if (this->get_tile_pos() != QPoint(-1, -1)) {
		return map::get()->get_tile(this->get_tile_pos());
	}

	return nullptr;
}

bool site_game_data::is_coastal() const
{
	if (!this->is_on_map()) {
		return false;
	}

	return map::get()->is_tile_coastal(this->get_tile_pos());
}

bool site_game_data::is_near_water() const
{
	if (!this->is_on_map()) {
		return false;
	}

	return map::get()->is_tile_near_water(this->get_tile_pos());
}

bool site_game_data::has_route() const
{
	const tile *tile = this->get_tile();

	if (tile == nullptr) {
		return false;
	}

	return tile->has_route();
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

	return this->get_province()->get_game_data()->get_provincial_capital() == this->site;
}

bool site_game_data::is_capital() const
{
	if (this->get_owner() == nullptr) {
		return false;
	}

	return this->get_owner()->get_game_data()->get_capital() == this->site;
}

bool site_game_data::can_be_capital() const
{
	assert_throw(this->site->is_settlement());

	if (!this->is_built()) {
		return false;
	}

	if (!this->is_near_water()) {
		//settlements need to have sea access to be capitals, so that the country's transport network can include water access, and so that the country can trade in the world market
		return false;
	}

	return true;
}

void site_game_data::set_owner(const country *owner)
{
	if (owner == this->get_owner()) {
		return;
	}

	const country *old_owner = this->get_owner();

	if (old_owner != nullptr) {
		if (this->site->is_settlement()) {
			this->population->remove_upper_population(old_owner->get_game_data()->get_population());

			for (const auto &[building, commodity_bonuses] : old_owner->get_game_data()->get_building_commodity_bonuses()) {
				if (!this->has_building(building)) {
					continue;
				}

				for (const auto &[commodity, bonus] : commodity_bonuses) {
					this->change_base_commodity_output(commodity, -centesimal_int(bonus));
				}
			}
		}

		old_owner->get_game_data()->on_site_gained(this->site, -1);

		for (const auto &[commodity, output] : this->get_commodity_outputs()) {
			if (commodity->is_local()) {
				continue;
			}

			old_owner->get_game_data()->change_transportable_commodity_output(commodity, -this->get_transportable_commodity_output(commodity));
		}
	}

	this->owner = owner;

	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		population_unit->set_country(owner);
	}

	if (this->get_owner() != nullptr) {
		for (const auto &[commodity, output] : this->get_commodity_outputs()) {
			if (commodity->is_local()) {
				continue;
			}

			this->get_owner()->get_game_data()->change_transportable_commodity_output(commodity, this->get_transportable_commodity_output(commodity));
		}

		if (this->site->is_settlement()) {
			this->population->add_upper_population(this->get_owner()->get_game_data()->get_population());

			for (const auto &[building, commodity_bonuses] : this->get_owner()->get_game_data()->get_building_commodity_bonuses()) {
				if (!this->has_building(building)) {
					continue;
				}

				for (const auto &[commodity, bonus] : commodity_bonuses) {
					this->change_base_commodity_output(commodity, centesimal_int(bonus));
				}
			}
		}

		this->get_owner()->get_game_data()->on_site_gained(this->site, 1);
	}

	if (this->site->is_settlement() && this->is_built()) {
		this->check_building_conditions();
		this->check_free_buildings();
	}

	if (game::get()->is_running()) {
		emit owner_changed();
	}
}

const std::string &site_game_data::get_current_cultural_name() const
{
	return this->site->get_cultural_name(this->get_culture());
}

void site_game_data::set_culture(const metternich::culture *culture)
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
}

void site_game_data::on_population_main_culture_changed(const metternich::culture *culture)
{
	if (culture != nullptr) {
		this->set_culture(culture);
	} else if (this->get_province() != nullptr) {
		this->set_culture(this->get_province()->get_game_data()->get_culture());
	} else {
		this->set_culture(nullptr);
	}
}

void site_game_data::set_religion(const metternich::religion *religion)
{
	if (religion == this->get_religion()) {
		return;
	}

	this->religion = religion;

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_diplomatic_map_mode_dirty(diplomatic_map_mode::religious);
		}
	}

	emit religion_changed();
}

void site_game_data::on_population_main_religion_changed(const metternich::religion *religion)
{
	if (religion != nullptr) {
		this->set_religion(religion);
	} else if (this->get_province() != nullptr) {
		this->set_religion(this->get_province()->get_game_data()->get_religion());
	} else {
		this->set_religion(nullptr);
	}
}

void site_game_data::set_settlement_type(const metternich::settlement_type *settlement_type)
{
	assert_throw(this->site->is_settlement());

	if (settlement_type == this->get_settlement_type()) {
		return;
	}

	const metternich::settlement_type *old_settlement_type = this->get_settlement_type();

	this->settlement_type = settlement_type;

	if (old_settlement_type == nullptr && this->get_settlement_type() != nullptr) {
		this->on_settlement_built(1);

		if (this->get_owner() != nullptr) {
			if (this->get_owner()->get_game_data()->get_capital() == nullptr && this->can_be_capital()) {
				this->get_owner()->get_game_data()->set_capital(this->site);
			}
		}

		if (this->get_province()->get_game_data()->get_provincial_capital() == nullptr) {
			this->get_province()->get_game_data()->set_provincial_capital(this->site);
		}
	} else if (old_settlement_type != nullptr && this->get_settlement_type() == nullptr) {
		this->on_settlement_built(-1);

		if (this->get_owner() != nullptr) {
			if (this->get_owner()->get_game_data()->get_capital() == this->site) {
				this->get_owner()->get_game_data()->choose_capital();
			}
		}

		if (this->get_province()->get_game_data()->get_provincial_capital() == this->site) {
			this->get_province()->get_game_data()->choose_provincial_capital();
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

const resource *site_game_data::get_resource() const
{
	return this->site->get_map_data()->get_resource();
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

const building_type *site_game_data::get_building_class_type(const building_class *building_class) const
{
	if (this->get_culture() == nullptr) {
		return nullptr;
	}

	return this->get_culture()->get_building_class_type(building_class);
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

bool site_game_data::has_building_class(const building_class *building_class) const
{
	const building_type *building = this->get_slot_building(building_class->get_slot_type());

	if (building == nullptr) {
		return false;
	}

	return building->get_building_class() == building_class;
}

bool site_game_data::can_gain_building(const building_type *building) const
{
	return this->get_building_slot(building->get_slot_type())->can_gain_building(building);
}

bool site_game_data::can_gain_building_class(const building_class *building_class) const
{
	const building_type *building = this->get_building_class_type(building_class);

	if (building == nullptr) {
		return false;
	}

	return this->can_gain_building(building);
}

void site_game_data::add_building(const building_type *building)
{
	this->get_building_slot(building->get_slot_type())->set_building(building);
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
			if (building_slot->can_maintain_building(building)) {
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

	if (this->get_culture() == nullptr) {
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

	const resource *resource = this->get_resource();
	const int free_resource_building_level = this->get_settlement_type()->get_free_resource_building_level();
	if (resource != nullptr && free_resource_building_level > 0) {
		for (const building_type *building : resource->get_buildings()) {
			if (building->get_resource_level() > free_resource_building_level) {
				continue;
			}

			if (this->check_free_building(building)) {
				changed = true;
			}
		}
	}

	if (this->is_capital()) {
		for (const building_type *building : building_type::get_all()) {
			if (!building->is_free_in_capital()) {
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
	if (building != this->get_culture()->get_building_class_type(building->get_building_class())) {
		return false;
	}

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

	if (building->get_required_technology() != nullptr && (this->get_owner() == nullptr || !this->get_owner()->get_game_data()->has_technology(building->get_required_technology()))) {
		return false;
	}

	building_slot->set_building(building);
	return true;
}

void site_game_data::on_settlement_built(const int multiplier)
{
	this->get_province()->get_game_data()->change_settlement_count(multiplier);

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_settlement_count(multiplier);
	}

	if (this->get_province() != nullptr && this->get_resource() != nullptr) {
		for (const auto &[commodity, value] : this->get_province()->get_game_data()->get_improved_resource_commodity_bonuses(this->get_resource())) {
			this->change_base_commodity_output(commodity, centesimal_int(value) * multiplier);
		}
	}
}

void site_game_data::on_building_gained(const building_type *building, const int multiplier)
{
	assert_throw(building != nullptr);
	assert_throw(multiplier != 0);
	assert_throw(this->get_province() != nullptr);

	if (this->get_owner() != nullptr) {
		country_game_data *country_game_data = this->get_owner()->get_game_data();
		country_game_data->change_settlement_building_count(building, multiplier);

		for (const auto &[commodity, bonus] : country_game_data->get_building_commodity_bonuses(building)) {
			this->change_base_commodity_output(commodity, centesimal_int(bonus) * multiplier);
		}
	}

	if (building->get_province_modifier() != nullptr) {
		building->get_province_modifier()->apply(this->get_province(), multiplier);
	}

	if (building->get_settlement_modifier() != nullptr) {
		building->get_settlement_modifier()->apply(this->site, multiplier);
	}
}

void site_game_data::on_wonder_gained(const wonder *wonder, const int multiplier)
{
	assert_throw(wonder != nullptr);
	assert_throw(multiplier != 0);

	if (this->get_owner() != nullptr && wonder->get_country_modifier() != nullptr) {
		wonder->get_country_modifier()->apply(this->get_owner(), multiplier);
	}

	if (wonder->get_province_modifier() != nullptr) {
		wonder->get_province_modifier()->apply(this->get_province(), multiplier);
	}
}

void site_game_data::on_improvement_gained(const improvement *improvement, const int multiplier)
{
	if (improvement->get_output_commodity() != nullptr) {
		this->change_base_commodity_output(improvement->get_output_commodity(), centesimal_int(improvement->get_output_multiplier()) * multiplier);
	}

	if (this->get_province() != nullptr && this->get_resource() != nullptr) {
		for (const auto &[commodity, value] : this->get_province()->get_game_data()->get_improved_resource_commodity_bonuses(this->get_resource())) {
			this->change_base_commodity_output(commodity, centesimal_int(value) * multiplier);
		}
	}

	emit improvement_changed();
}

QVariantList site_game_data::get_scripted_modifiers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_scripted_modifiers());
}

bool site_game_data::has_scripted_modifier(const scripted_site_modifier *modifier) const
{
	return this->get_scripted_modifiers().contains(modifier);
}

void site_game_data::add_scripted_modifier(const scripted_site_modifier *modifier, const int duration)
{
	const read_only_context ctx(this->site);

	this->scripted_modifiers[modifier] = std::max(this->scripted_modifiers[modifier], duration);

	if (modifier->get_modifier() != nullptr) {
		modifier->get_modifier()->apply(this->site);
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void site_game_data::remove_scripted_modifier(const scripted_site_modifier *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		modifier->get_modifier()->remove(this->site);
	}

	if (game::get()->is_running()) {
		emit scripted_modifiers_changed();
	}
}

void site_game_data::decrement_scripted_modifiers()
{
	if (this->scripted_modifiers.empty()) {
		return;
	}

	std::vector<const scripted_site_modifier *> modifiers_to_remove;
	for (auto &[modifier, duration] : this->scripted_modifiers) {
		--duration;

		if (duration == 0) {
			modifiers_to_remove.push_back(modifier);
		}
	}

	for (const scripted_site_modifier *modifier : modifiers_to_remove) {
		this->remove_scripted_modifier(modifier);
	}
}

void site_game_data::add_population_unit(qunique_ptr<population_unit> &&population_unit)
{
	this->get_population()->on_population_unit_gained(population_unit.get());

	this->population_units.push_back(std::move(population_unit));

	if (this->is_capital()) {
		//recalculate commodity outputs, because there could be a capital commodity population bonus
		this->calculate_commodity_outputs();
	}

	if (game::get()->is_running()) {
		emit population_units_changed();
	}
}

qunique_ptr<population_unit> site_game_data::pop_population_unit(population_unit *population_unit)
{
	for (size_t i = 0; i < this->population_units.size();) {
		if (this->population_units[i].get() == population_unit) {
			qunique_ptr<metternich::population_unit> population_unit_unique_ptr = std::move(this->population_units[i]);
			this->population_units.erase(this->population_units.begin() + i);

			population_unit->set_settlement(nullptr);

			this->get_population()->on_population_unit_lost(population_unit);

			if (this->is_capital()) {
				//recalculate commodity outputs, because there could be a capital commodity population bonus
				this->calculate_commodity_outputs();
			}

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

void site_game_data::clear_population_units()
{
	this->population_units.clear();
}

void site_game_data::create_population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const phenotype *phenotype)
{
	assert_throw(this->site->is_settlement());
	assert_throw(this->is_built());

	auto population_unit = make_qunique<metternich::population_unit>(type, culture, religion, phenotype, this->site);
	this->get_province()->get_game_data()->add_population_unit(population_unit.get());

	this->add_population_unit(std::move(population_unit));
}

void site_game_data::on_population_type_count_changed(const population_type *type, const int change)
{
	if (type->get_output_commodity() != nullptr) {
		this->change_base_commodity_output(type->get_output_commodity(), centesimal_int(type->get_output_value()) * change);
	}

	for (const auto &[commodity, value] : type->get_consumed_commodities()) {
		if (commodity->is_local()) {
			this->change_local_commodity_consumption(commodity, value * change);
		}
	}
}

void site_game_data::change_profession_capacity(const profession *profession, const int change)
{
	if (change == 0) {
		return;
	}

	assert_throw(this->site->is_settlement());
	assert_throw(this->is_built());

	const int count = (this->profession_capacities[profession] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->profession_capacities.erase(profession);
	}

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_profession_capacity(profession, change);
	}
}

void site_game_data::change_housing(const int change)
{
	if (change == 0) {
		return;
	}

	this->housing += change;

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_housing(change);
	}

	emit housing_changed();
}

void site_game_data::change_base_commodity_output(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	const centesimal_int &count = (this->base_commodity_outputs[commodity] += change);

	assert_throw(count >= 0);

	if (count == 0) {
		this->base_commodity_outputs.erase(commodity);
	}

	this->calculate_commodity_outputs();
}

QVariantList site_game_data::get_commodity_outputs_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_commodity_outputs());
}

void site_game_data::set_commodity_output(const commodity *commodity, const centesimal_int &output)
{
	assert_throw(output >= 0);

	const centesimal_int old_output = this->get_commodity_output(commodity);
	if (output == old_output) {
		return;
	}

	const centesimal_int old_transportable_output = this->get_transportable_commodity_output(commodity);

	if (output == 0) {
		this->commodity_outputs.erase(commodity);
	} else {
		this->commodity_outputs[commodity] = output;
	}

	const centesimal_int transportable_output = this->get_transportable_commodity_output(commodity);
	const centesimal_int transportable_change = transportable_output - old_transportable_output;

	if (commodity->is_local()) {
		if (this->get_province() != nullptr) {
			this->get_province()->get_game_data()->change_local_commodity_output(commodity, transportable_change);
		}
	} else {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_game_data()->change_transportable_commodity_output(commodity, transportable_change);
		}
	}

	emit commodity_outputs_changed();
}

void site_game_data::calculate_commodity_outputs()
{
	commodity_map<centesimal_int> outputs = this->base_commodity_outputs;

	for (const auto &[commodity, output] : this->get_commodity_outputs()) {
		//ensure the current outputs are in the new map, so that they get removed if no longer present
		outputs[commodity];
	}

	if (this->get_owner() != nullptr && this->is_capital()) {
		for (const auto &[commodity, value] : this->get_owner()->get_game_data()->get_capital_commodity_bonuses()) {
			outputs[commodity] += value;
		}

		for (const auto &[commodity, value] : this->get_owner()->get_game_data()->get_capital_commodity_bonuses_per_population()) {
			outputs[commodity] += (value * this->get_population_unit_count());
		}
	}

	int output_modifier = this->get_output_modifier();
	commodity_map<int> commodity_output_modifiers = this->get_commodity_output_modifiers();

	const province *province = this->get_province();
	if (province != nullptr) {
		for (auto &[commodity, output] : outputs) {
			for (const auto &[threshold, bonus] : province->get_game_data()->get_commodity_bonus_for_tile_threshold_map(commodity)) {
				if (output >= threshold) {
					output += bonus;
				}
			}
		}

		output_modifier += province->get_game_data()->get_output_modifier();

		for (const auto &[commodity, modifier] : province->get_game_data()->get_commodity_output_modifiers()) {
			commodity_output_modifiers[commodity] += modifier;
		}
	}

	if (this->get_owner() != nullptr) {
		output_modifier += this->get_owner()->get_game_data()->get_output_modifier();
		if (this->get_resource() != nullptr && this->get_resource()->get_commodity() != nullptr) {
			commodity_output_modifiers[this->get_resource()->get_commodity()] += this->get_owner()->get_game_data()->get_resource_output_modifier();
		}

		for (const auto &[commodity, modifier] : this->get_owner()->get_game_data()->get_commodity_output_modifiers()) {
			commodity_output_modifiers[commodity] += modifier;
		}
	}

	for (auto &[commodity, output] : outputs) {
		const int modifier = output_modifier + commodity_output_modifiers[commodity];

		if (modifier != 0) {
			output *= 100 + modifier;
			output /= 100;
		}

		this->set_commodity_output(commodity, output);
	}
}

void site_game_data::change_local_commodity_consumption(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	log_trace(std::format("Changing local consumption in settlement {} of commodity {} (currently {}) by {}.", this->site->get_identifier(), commodity->get_identifier(), this->get_local_commodity_consumption(commodity).to_string(), change.to_string()));

	const centesimal_int count = (this->local_commodity_consumptions[commodity] += change);

	log_trace(std::format("Changed local consumption in settlement {} of commodity {} by {}, making it now {}.", this->site->get_identifier(), commodity->get_identifier(), change.to_string(), this->get_local_commodity_consumption(commodity).to_string()));

	assert_throw(count >= 0);

	if (count == 0) {
		this->local_commodity_consumptions.erase(commodity);
	}
}

void site_game_data::set_depot_level(const int level)
{
	if (level == this->get_depot_level()) {
		return;
	}

	this->depot_level = level;

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_transport_level_recalculation_needed(true);
		}
	}
}

void site_game_data::set_port_level(const int level)
{
	if (level == this->get_port_level()) {
		return;
	}

	this->port_level = level;

	if (game::get()->is_running()) {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_turn_data()->set_transport_level_recalculation_needed(true);
		}
	}
}

void site_game_data::set_transport_level(const int level)
{
	if (level == this->get_transport_level()) {
		return;
	}

	commodity_map<centesimal_int> old_transportable_outputs;
	for (auto &[commodity, output] : this->get_commodity_outputs()) {
		old_transportable_outputs[commodity] = this->get_transportable_commodity_output(commodity);
	}

	this->transport_level = level;

	for (const auto &[commodity, old_transportable_output] : old_transportable_outputs) {
		const centesimal_int transportable_output = this->get_transportable_commodity_output(commodity);
		const centesimal_int transportable_change = transportable_output - old_transportable_output;

		if (commodity->is_local()) {
			if (this->get_province() != nullptr) {
				this->get_province()->get_game_data()->change_local_commodity_output(commodity, transportable_change);
			}
		} else {
			if (this->get_owner() != nullptr) {
				this->get_owner()->get_game_data()->change_transportable_commodity_output(commodity, transportable_change);
			}
		}
	}

	if (game::get()->is_running()) {
		emit transport_level_changed();
	}
}

void site_game_data::set_sea_transport_level(const int level)
{
	if (level == this->get_sea_transport_level()) {
		return;
	}

	commodity_map<centesimal_int> old_transportable_outputs;
	for (auto &[commodity, output] : this->get_commodity_outputs()) {
		old_transportable_outputs[commodity] = this->get_transportable_commodity_output(commodity);
	}

	this->sea_transport_level = level;

	for (const auto &[commodity, old_transportable_output] : old_transportable_outputs) {
		const centesimal_int transportable_output = this->get_transportable_commodity_output(commodity);
		const centesimal_int transportable_change = transportable_output - old_transportable_output;

		if (commodity->is_local()) {
			if (this->get_province() != nullptr) {
				this->get_province()->get_game_data()->change_local_commodity_output(commodity, transportable_change);
			}
		} else {
			if (this->get_owner() != nullptr) {
				this->get_owner()->get_game_data()->change_transportable_commodity_output(commodity, transportable_change);
			}
		}
	}

	if (game::get()->is_running()) {
		emit transport_level_changed();
	}
}

centesimal_int site_game_data::get_transportable_commodity_output(const commodity *commodity) const
{
	const centesimal_int &output = this->get_commodity_output(commodity);

	if (commodity->is_abstract()) {
		return output;
	}

	return centesimal_int::min(output, centesimal_int(this->get_best_transport_level()));
}

bool site_game_data::can_be_visited() const
{
	return this->get_improvement() != nullptr && this->get_improvement()->is_ruins();
}

QVariantList site_game_data::get_visiting_armies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_visiting_armies());
}

}
