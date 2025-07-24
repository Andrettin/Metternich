#include "metternich.h"

#include "map/site_game_data.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/character_role.h"
#include "country/country.h"
#include "country/country_economy.h"
#include "country/country_game_data.h"
#include "country/country_turn_data.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/country_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/improvement_slot.h"
#include "infrastructure/settlement_building_slot.h"
#include "infrastructure/settlement_type.h"
#include "infrastructure/wonder.h"
#include "map/diplomatic_map_mode.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_map_data.h"
#include "map/site_tier.h"
#include "map/site_type.h"
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "population/profession.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "script/scripted_site_modifier.h"
#include "ui/portrait.h"
#include "unit/army.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

site_game_data::site_game_data(const metternich::site *site) : site(site)
{
	if (site->is_settlement()) {
		this->initialize_building_slots();
		this->free_food_consumption = site_game_data::settlement_base_free_food_consumption;
	}

	const resource *resource = this->get_resource();
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

	connect(this, &site_game_data::owner_changed, this, &site_game_data::title_name_changed);
	connect(this, &site_game_data::culture_changed, this, &site_game_data::title_name_changed);
	connect(this, &site_game_data::religion_changed, this, &site_game_data::title_name_changed);

	connect(this, &site_game_data::owner_changed, this, &site_game_data::landholder_title_name_changed);
	connect(this, &site_game_data::culture_changed, this, &site_game_data::landholder_title_name_changed);
	connect(this, &site_game_data::religion_changed, this, &site_game_data::landholder_title_name_changed);
	connect(this, &site_game_data::landholder_changed, this, &site_game_data::landholder_title_name_changed);
}

void site_game_data::initialize_resource()
{
	const resource *resource = this->get_resource();
	if (resource == nullptr) {
		return;
	}

	if (resource->get_modifier() != nullptr) {
		resource->get_modifier()->apply(this->site);
	}
}

void site_game_data::do_turn()
{
	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		population_unit->do_turn();
	}

	this->decrement_scripted_modifiers();
}

void site_game_data::do_everyday_consumption()
{
	std::vector<population_unit *> shuffled_population_units;
	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		shuffled_population_units.push_back(population_unit.get());
	}
	vector::shuffle(shuffled_population_units);

	std::vector<population_unit *> population_units;
	for (population_unit *population_unit : shuffled_population_units) {
		if (population_unit->is_everyday_consumption_fulfilled()) {
			population_units.push_back(population_unit);
		} else {
			population_units.insert(population_units.begin(), population_unit);
		}
	}

	for (const auto &[commodity, consumption] : this->local_everyday_consumption) {
		assert_throw(commodity->is_local() && !commodity->is_provincial());
		assert_throw(!commodity->is_storable());

		const int effective_consumption = std::min(consumption.to_int(), this->get_commodity_output(commodity).to_int());

		centesimal_int remaining_consumption(consumption.to_int() - effective_consumption);
		if (remaining_consumption == 0) {
			continue;
		}

		//go through population units belonging to the settlement in random order, and cause the effects of them not being able to have their consumption fulfilled
		for (population_unit *population_unit : population_units) {
			const centesimal_int pop_consumption = population_unit->get_type()->get_everyday_consumption(commodity);
			if (pop_consumption == 0) {
				continue;
			}

			population_unit->set_everyday_consumption_fulfilled(false);
			remaining_consumption -= pop_consumption;

			if (remaining_consumption <= 0) {
				break;
			}
		}
	}
}

void site_game_data::do_luxury_consumption()
{
	std::vector<population_unit *> shuffled_population_units;
	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		shuffled_population_units.push_back(population_unit.get());
	}
	vector::shuffle(shuffled_population_units);

	std::vector<population_unit *> population_units;
	for (population_unit *population_unit : shuffled_population_units) {
		if (population_unit->is_luxury_consumption_fulfilled()) {
			population_units.push_back(population_unit);
		} else {
			population_units.insert(population_units.begin(), population_unit);
		}
	}

	for (const auto &[commodity, consumption] : this->local_luxury_consumption) {
		assert_throw(commodity->is_local() && !commodity->is_provincial());
		assert_throw(!commodity->is_storable());

		const int effective_consumption = std::min(consumption.to_int(), this->get_commodity_output(commodity).to_int());

		centesimal_int remaining_consumption(consumption.to_int() - effective_consumption);
		if (remaining_consumption == 0) {
			continue;
		}

		//go through population units belonging to the settlement in random order, and cause the effects of them not being able to have their consumption fulfilled
		for (population_unit *population_unit : population_units) {
			const centesimal_int pop_consumption = population_unit->get_type()->get_luxury_consumption(commodity);
			if (pop_consumption == 0) {
				continue;
			}

			population_unit->set_luxury_consumption_fulfilled(false);
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
	return this->site->get_map_data()->get_tile();
}

bool site_game_data::is_coastal() const
{
	return this->site->get_map_data()->is_coastal();
}

bool site_game_data::is_near_water() const
{
	return this->site->get_map_data()->is_near_water();
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
	return this->site->get_map_data()->get_province();
}

bool site_game_data::is_provincial_capital() const
{
	if (this->get_province() == nullptr) {
		return false;
	}

	return this->get_province()->get_provincial_capital() == this->site;
}

bool site_game_data::is_capital() const
{
	if (this->get_owner() == nullptr) {
		return false;
	}

	const metternich::site *owner_capital = this->get_owner()->get_game_data()->get_capital();
	const bool is_capital = owner_capital == this->site;
	return is_capital;
}

bool site_game_data::can_be_capital() const
{
	assert_throw(this->site->is_settlement());

	if (!this->is_built()) {
		return false;
	}

	if (!this->is_near_water() && !this->site->is_celestial_body()) {
		//settlements need to have sea access to be capitals, so that the country's transport network can include water access, and so that the country can trade in the world market
		return false;
	}

	return true;
}

site_tier site_game_data::get_tier() const
{
	site_tier tier = site_tier::none;

	if (this->get_settlement_type() != nullptr) {
		tier = static_cast<site_tier>(this->get_settlement_type()->get_level());
	} else if (this->get_resource_improvement() != nullptr) {
		tier = static_cast<site_tier>(this->get_resource_improvement()->get_level());
	}

	if (tier > this->site->get_max_tier()) {
		tier = this->site->get_max_tier();
	}

	return tier;
}

const std::string &site_game_data::get_title_name() const
{
	const site_tier tier = this->get_tier();
	return this->site->get_title_name(this->get_owner() ? this->get_owner()->get_game_data()->get_government_type() : nullptr, tier, this->get_culture());
}

const std::string &site_game_data::get_landholder_title_name() const
{
	const site_tier tier = this->get_tier();
	const gender gender = this->get_landholder() != nullptr ? this->get_landholder()->get_gender() : gender::male;
	return this->site->get_landholder_title_name(this->get_owner() ? this->get_owner()->get_game_data()->get_government_type() : nullptr, tier, gender, this->get_culture());
}

void site_game_data::set_owner(const country *owner)
{
	if (owner == this->get_owner()) {
		return;
	}

	const country *old_owner = this->get_owner();

	if (old_owner != nullptr) {
		for (const auto &[improvement, commodity_bonuses] : old_owner->get_economy()->get_improvement_commodity_bonuses()) {
			if (!this->has_improvement(improvement)) {
				continue;
			}

			for (const auto &[commodity, bonus] : commodity_bonuses) {
				this->change_base_commodity_output(commodity, -bonus);
			}
		}

		if (this->can_have_population()) {
			this->population->remove_upper_population(old_owner->get_game_data()->get_population());
		}

		if (this->site->is_settlement()) {
			for (const auto &[commodity, bonus] : old_owner->get_economy()->get_settlement_commodity_bonuses()) {
				this->change_base_commodity_output(commodity, -bonus);
			}

			for (const auto &[building, commodity_bonuses] : old_owner->get_economy()->get_building_commodity_bonuses()) {
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

			old_owner->get_economy()->change_transportable_commodity_output(commodity, -this->get_transportable_commodity_output(commodity));
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

			this->get_owner()->get_economy()->change_transportable_commodity_output(commodity, this->get_transportable_commodity_output(commodity));
		}

		for (const auto &[improvement, commodity_bonuses] : this->get_owner()->get_economy()->get_improvement_commodity_bonuses()) {
			if (!this->has_improvement(improvement)) {
				continue;
			}

			for (const auto &[commodity, bonus] : commodity_bonuses) {
				this->change_base_commodity_output(commodity, bonus);
			}
		}

		if (this->can_have_population()) {
			this->population->add_upper_population(this->get_owner()->get_game_data()->get_population());
		}

		if (this->site->is_settlement()) {
			for (const auto &[commodity, bonus] : this->get_owner()->get_economy()->get_settlement_commodity_bonuses()) {
				this->change_base_commodity_output(commodity, bonus);
			}

			for (const auto &[building, commodity_bonuses] : this->get_owner()->get_economy()->get_building_commodity_bonuses()) {
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

	if (old_settlement_type != nullptr) {
		if (old_settlement_type->get_modifier() != nullptr) {
			old_settlement_type->get_modifier()->apply(this->site, -1);
		}
	}

	this->settlement_type = settlement_type;

	if (old_settlement_type == nullptr && this->get_settlement_type() != nullptr) {
		this->on_settlement_built(1);

		if (this->get_owner() != nullptr) {
			if (this->get_owner()->get_game_data()->get_capital() == nullptr && this->can_be_capital()) {
				this->get_owner()->get_game_data()->set_capital(this->site);
			}
		}
	} else if (old_settlement_type != nullptr && this->get_settlement_type() == nullptr) {
		this->on_settlement_built(-1);

		if (this->get_owner() != nullptr) {
			if (this->get_owner()->get_game_data()->get_capital() == this->site) {
				this->get_owner()->get_game_data()->choose_capital();
			}
		}
	}

	if (this->get_settlement_type() != nullptr) {
		if (this->get_settlement_type()->get_modifier() != nullptr) {
			this->get_settlement_type()->get_modifier()->apply(this->site, 1);
		}

		this->check_building_conditions();
		this->check_free_buildings();
	}

	if (game::get()->is_running()) {
		emit settlement_type_changed();
		emit map::get()->tile_settlement_type_changed(this->get_tile_pos());
	}
}

void site_game_data::check_settlement_type()
{
	if (this->get_settlement_type() == nullptr) {
		return;
	}

	if (this->get_settlement_type()->get_conditions() == nullptr || this->get_settlement_type()->get_conditions()->check(this->site, read_only_context(this->site))) {
		return;
	}

	const std::vector<const metternich::settlement_type *> potential_settlement_types = this->get_best_settlement_types(this->get_settlement_type()->get_base_settlement_types());

	assert_throw(!potential_settlement_types.empty());
	this->set_settlement_type(vector::get_random(potential_settlement_types));
}

std::vector<const metternich::settlement_type *> site_game_data::get_best_settlement_types(const std::vector<const metternich::settlement_type *> &settlement_types) const
{
	std::vector<const metternich::settlement_type *> potential_settlement_types;

	int best_preserved_building_count = 0;

	for (const metternich::settlement_type *base_settlement_type : settlement_types) {
		if (base_settlement_type->get_conditions() != nullptr && !base_settlement_type->get_conditions()->check(this->site, read_only_context(this->site))) {
			continue;
		}

		int preserved_building_count = 0;
		for (const qunique_ptr<settlement_building_slot> &building_slot : this->building_slots) {
			if (building_slot->get_building() == nullptr) {
				continue;
			}

			if (vector::contains(building_slot->get_building()->get_settlement_types(), base_settlement_type)) {
				++preserved_building_count;
			}
		}

		if (preserved_building_count < best_preserved_building_count) {
			continue;
		} else if (preserved_building_count > best_preserved_building_count) {
			potential_settlement_types.clear();
			best_preserved_building_count = preserved_building_count;
		}

		potential_settlement_types.push_back(base_settlement_type);
	}

	if (potential_settlement_types.empty()) {
		std::vector<const metternich::settlement_type *> base_settlement_types;

		for (const metternich::settlement_type *settlement_type : settlement_types) {
			for (const metternich::settlement_type *base_settlement_type : settlement_type->get_base_settlement_types()) {
				if (!vector::contains(base_settlement_types, base_settlement_type)) {
					base_settlement_types.push_back(base_settlement_type);
				}
			}
		}

		return this->get_best_settlement_types(base_settlement_types);
	}

	return potential_settlement_types;
}

bool site_game_data::is_built() const
{
	if (this->site->is_settlement()) {
		return this->get_settlement_type() != nullptr;
	} else {
		return this->get_main_improvement() != nullptr && !this->get_main_improvement()->is_visitable();
	}
}

const resource *site_game_data::get_resource() const
{
	return this->site->get_map_data()->get_resource();
}

const improvement *site_game_data::get_main_improvement() const
{
	const improvement *main_improvement = this->get_improvement(improvement_slot::main);

	if (main_improvement != nullptr) {
		return main_improvement;
	}

	return this->get_improvement(improvement_slot::resource);
}

const improvement *site_game_data::get_resource_improvement() const
{
	return this->get_improvement(improvement_slot::resource);
}

const improvement *site_game_data::get_depot_improvement() const
{
	return this->get_improvement(improvement_slot::depot);
}

const improvement *site_game_data::get_port_improvement() const
{
	return this->get_improvement(improvement_slot::port);
}

bool site_game_data::has_improvement(const improvement *improvement) const
{
	assert_throw(improvement != nullptr);

	return this->get_improvement(improvement->get_slot()) == improvement;
}

bool site_game_data::has_improvement_or_better(const improvement *improvement) const
{
	if (this->has_improvement(improvement)) {
		return true;
	}

	for (const metternich::improvement *requiring_improvement : improvement->get_requiring_improvements()) {
		if (this->has_improvement_or_better(requiring_improvement)) {
			return true;
		}
	}

	return false;
}

void site_game_data::set_improvement(const improvement_slot slot, const improvement *improvement)
{
	const metternich::improvement *old_improvement = this->get_improvement(slot);

	if (old_improvement == improvement) {
		return;
	}

	if (old_improvement != nullptr) {
		this->on_improvement_gained(old_improvement, -1);
	}

	this->improvements[slot] = improvement;

	if (improvement != nullptr) {
		this->on_improvement_gained(improvement, 1);
	}

	if (slot == improvement_slot::main || (slot == improvement_slot::resource && this->get_improvement(improvement_slot::main) == nullptr)) {
		this->get_tile()->on_main_improvement_changed();
	}

	emit improvements_changed();
	emit map::get()->tile_improvement_changed(this->get_tile_pos());
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

bool site_game_data::has_building_class_or_better(const building_class *building_class) const
{
	const building_type *slot_building = this->get_slot_building(building_class->get_slot_type());

	while (slot_building != nullptr) {
		if (slot_building->get_building_class() == building_class) {
			return true;
		}

		slot_building = slot_building->get_required_building();
	}

	return false;
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

		const int building_level = building->get_level();

		const wonder *wonder = building_slot->get_wonder();
		if (wonder != nullptr && !building_slot->can_have_wonder(wonder)) {
			building_slot->set_wonder(nullptr);
		}

		//if the building fails its conditions, try to replace it with one of its required buildings, if valid
		while (building != nullptr) {
			if (building_slot->can_maintain_building(building)) {
				break;
			}

			building = building->get_required_building();
		}

		//try to place a building of equivalent level for the same slot which has its conditions fulfilled
		if (building == nullptr) {
			std::vector<const building_type *> potential_buildings;

			for (const building_type *slot_building : building_slot->get_type()->get_building_types()) {
				if (slot_building->get_required_building() != nullptr) {
					continue;
				}

				if (!building_slot->can_maintain_building(slot_building)) {
					continue;
				}

				potential_buildings.push_back(slot_building);
			}

			if (!potential_buildings.empty()) {
				building = vector::get_random(potential_buildings);
			}
		}

		if (building != nullptr && building->get_level() < building_level) {
			for (int i = building->get_level() + 1; i <= building_level; ++i) {
				std::vector<const building_type *> potential_buildings;

				for (const building_type *requiring_building : building->get_requiring_buildings()) {
					if (!building_slot->can_maintain_building(requiring_building)) {
						continue;
					}

					potential_buildings.push_back(requiring_building);
				}

				if (potential_buildings.empty()) {
					break;
				}

				building = vector::get_random(potential_buildings);
				assert_throw(building->get_level() == i);
			}
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
	const int free_resource_improvement_level = this->get_settlement_type()->get_free_resource_improvement_level();
	if (resource != nullptr && free_resource_improvement_level > 0) {
		for (const improvement *improvement : resource->get_improvements()) {
			if (improvement->get_level() > free_resource_improvement_level) {
				continue;
			}

			if (this->check_free_improvement(improvement)) {
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

		//capitals get the highest level of depot and port improvements for free
		for (const improvement *improvement : improvement::get_all()) {
			if (improvement->get_slot() != improvement_slot::depot && improvement->get_slot() != improvement_slot::port) {
				continue;
			}

			if (this->check_free_improvement(improvement)) {
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

bool site_game_data::check_free_improvement(const improvement *improvement)
{
	if (improvement->get_required_technology() != nullptr && (this->get_owner() == nullptr || !this->get_owner()->get_game_data()->has_technology(improvement->get_required_technology()))) {
		return false;
	}

	if (!improvement->is_buildable_on_site(this->site)) {
		return false;
	}

	this->set_improvement(improvement->get_slot(), improvement);

	return true;
}

void site_game_data::on_settlement_built(const int multiplier)
{
	this->get_province()->get_game_data()->change_settlement_count(multiplier);

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_settlement_count(multiplier);
	}

	if (this->get_province() != nullptr) {
		for (const auto &[commodity, bonus] : defines::get()->get_settlement_commodity_bonuses()) {
			this->change_base_commodity_output(commodity, centesimal_int(bonus));
		}

		const tile *tile = this->get_tile();
		if (tile != nullptr && tile->has_river()) {
			for (const auto &[commodity, bonus] : defines::get()->get_river_settlement_commodity_bonuses()) {
				this->change_base_commodity_output(commodity, centesimal_int(bonus));
			}
		}

		if (this->get_owner() != nullptr) {
			for (const auto &[commodity, bonus] : this->get_owner()->get_economy()->get_settlement_commodity_bonuses()) {
				this->change_base_commodity_output(commodity, bonus);
			}
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
		country_economy *country_economy = this->get_owner()->get_economy();
		country_game_data->change_settlement_building_count(building, multiplier);

		for (const auto &[commodity, bonus] : country_economy->get_building_commodity_bonuses(building)) {
			this->change_base_commodity_output(commodity, centesimal_int(bonus) * multiplier);
		}
	}

	if (building->get_province_modifier() != nullptr) {
		building->get_province_modifier()->apply(this->get_province(), multiplier);
	}

	if (building->get_settlement_modifier() != nullptr) {
		building->get_settlement_modifier()->apply(this->site, multiplier);
	}

	if (multiplier > 0 && building->get_effects() != nullptr) {
		context effects_ctx(this->site);
		building->get_effects()->do_effects(this->site, effects_ctx);
	}
}

void site_game_data::on_wonder_gained(const wonder *wonder, const int multiplier)
{
	assert_throw(wonder != nullptr);
	assert_throw(multiplier != 0);

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->on_wonder_gained(wonder, multiplier);
	}

	if (wonder->get_province_modifier() != nullptr) {
		wonder->get_province_modifier()->apply(this->get_province(), multiplier);
	}
}

void site_game_data::on_improvement_gained(const improvement *improvement, const int multiplier)
{
	if (this->get_province() != nullptr && this->get_resource() != nullptr && improvement->get_slot() == improvement_slot::resource) {
		for (const auto &[commodity, value] : this->get_province()->get_game_data()->get_improved_resource_commodity_bonuses(this->get_resource())) {
			this->change_base_commodity_output(commodity, centesimal_int(value) * multiplier);
		}
	}

	if (!improvement->get_employment_professions().empty()) {
		assert_throw(this->get_resource() != nullptr);
		assert_throw(improvement->get_slot() == improvement_slot::resource);
		this->change_production_capacity(improvement->get_production_capacity() * multiplier);
	}

	if (this->get_owner() != nullptr) {
		const country_economy *country_economy = this->get_owner()->get_economy();

		for (const auto &[commodity, bonus] : country_economy->get_improvement_commodity_bonuses(improvement)) {
			this->change_base_commodity_output(commodity, bonus * multiplier);
		}
	}

	if (this->get_resource() != nullptr && improvement->get_slot() == improvement_slot::resource) {
		if (this->get_resource()->get_improved_modifier() != nullptr) {
			this->get_resource()->get_improved_modifier()->apply(this->site, multiplier);
		}

		if (this->get_resource()->get_improved_country_modifier() != nullptr && this->get_owner() != nullptr) {
			this->get_resource()->get_improved_country_modifier()->apply(this->get_owner(), multiplier);
		}
	}

	if (improvement->get_modifier() != nullptr) {
		improvement->get_modifier()->apply(this->site, multiplier);
	}

	if (improvement->get_country_modifier() != nullptr && this->get_owner() != nullptr) {
		improvement->get_country_modifier()->apply(this->get_owner(), multiplier);
	}
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

bool site_game_data::can_have_population() const
{
	return this->site->is_settlement() || this->site->get_map_data()->get_type() == site_type::resource || (this->site->get_map_data()->get_type() == site_type::celestial_body && this->get_resource() != nullptr);
}

bool site_game_data::can_have_population_type(const population_type *type) const
{
	assert_throw(this->can_have_population());
	assert_throw(this->is_built());

	if (this->site->is_settlement()) {
		return this->get_settlement_type()->can_have_population_type(type);
	} else {
		return this->get_main_improvement()->can_have_population_type(type);
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

			population_unit->set_employment_location(nullptr, nullptr, true);
			population_unit->set_site(nullptr);

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
	assert_throw(type != nullptr);
	assert_throw(type->is_enabled());
	assert_throw(this->can_have_population());
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

	for (const auto &[commodity, value] : type->get_everyday_consumption()) {
		if (commodity->is_local() && !commodity->is_provincial()) {
			this->change_local_everyday_consumption(commodity, value * change);
		}
	}

	for (const auto &[commodity, value] : type->get_luxury_consumption()) {
		if (commodity->is_local() && !commodity->is_provincial()) {
			this->change_local_luxury_consumption(commodity, value * change);
		}
	}
}

const population_class *site_game_data::get_default_population_class() const
{
	assert_throw(this->can_have_population());
	assert_throw(this->is_built());

	if (!this->site->is_settlement() && this->get_main_improvement()->get_default_population_class() != nullptr) {
		return this->get_main_improvement()->get_default_population_class();
	}

	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_game_data()->get_default_population_class();
	}

	return defines::get()->get_default_population_class();
}

const population_class *site_game_data::get_default_literate_population_class() const
{
	assert_throw(this->can_have_population());
	assert_throw(this->is_built());

	if (this->site->is_settlement()) {
		if (this->get_owner() != nullptr && !this->get_owner()->get_game_data()->is_tribal() && !this->get_owner()->get_game_data()->is_clade()) {
			return defines::get()->get_default_literate_population_class();
		}
	}

	return nullptr;
}

population_unit *site_game_data::choose_population_unit_for_reallocation() const
{
	if (this->site->is_settlement() && this->get_population_unit_count() == 1) {
		//do not remove a settlement's last population unit
		return nullptr;
	}

	std::vector<population_unit *> population_units;

	centesimal_int lowest_output_value = centesimal_int::from_value(std::numeric_limits<int64_t>::max());

	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		if (population_unit->get_type()->get_output_commodity() != nullptr) {
			if (!population_unit->get_type()->get_output_commodity()->is_labor()) {
				continue;
			}

			if (this->get_owner() != nullptr && this->get_owner()->get_economy()->get_net_commodity_output(population_unit->get_type()->get_output_commodity()) < population_unit->get_type()->get_output_value()) {
				continue;
			}
		}

		if (population_unit->get_type()->get_country_modifier() != nullptr) {
			continue;
		}

		centesimal_int output_value(0);

		const profession *profession = population_unit->get_profession();
		if (profession != nullptr) {
			output_value = population_unit->get_employment_location()->get_employee_commodity_outputs(population_unit->get_type(), profession)[population_unit->get_profession()->get_output_commodity()];
		} else {
			output_value = centesimal_int(population_unit->get_type()->get_output_value());
		}

		if (output_value > lowest_output_value) {
			continue;
		} else if (output_value < lowest_output_value) {
			lowest_output_value = output_value;
			population_units.clear();
		}

		population_units.push_back(population_unit.get());
	}

	if (population_units.empty()) {
		return nullptr;
	}

	return vector::get_random(population_units);
}

const site *site_game_data::get_employment_site() const
{
	return this->site;
}

const std::vector<const profession *> &site_game_data::get_employment_professions() const
{
	const improvement *resource_improvement = this->get_resource_improvement();

	if (resource_improvement != nullptr) {
		return resource_improvement->get_employment_professions();
	}

	static constexpr std::vector<const profession *> empty_vector;
	return empty_vector;
}

void site_game_data::change_housing(const centesimal_int &change)
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

void site_game_data::set_landholder(const character *landholder)
{
	if (landholder == this->get_landholder()) {
		return;
	}

	const character *old_landholder = this->get_landholder();

	if (old_landholder != nullptr) {
		old_landholder->get_game_data()->apply_landholder_modifier(this->site, -1);
		old_landholder->get_game_data()->set_country(nullptr);
	}

	this->landholder = landholder;

	if (this->get_landholder() != nullptr) {
		this->get_landholder()->get_game_data()->apply_landholder_modifier(this->site, 1);
		this->get_landholder()->get_game_data()->set_country(this->get_owner());
	}

	if (game::get()->is_running()) {
		emit landholder_changed();

		if (old_landholder != nullptr) {
			emit old_landholder->get_game_data()->landholder_changed();
		}

		if (landholder != nullptr) {
			emit landholder->get_game_data()->landholder_changed();
		}
	}
}

void site_game_data::check_landholder()
{
	if (this->get_owner() == nullptr || this->get_owner()->get_game_data()->is_under_anarchy() || !this->is_built() || this->get_resource() == nullptr) {
		this->set_landholder(nullptr);
		return;
	}

	//remove the landholder if they have become obsolete
	if (this->get_landholder() != nullptr && this->get_landholder()->get_obsolescence_technology() != nullptr && this->get_owner()->get_game_data()->has_technology(this->get_landholder()->get_obsolescence_technology())) {
		if (game::get()->is_running()) {
			if (this->get_owner() == game::get()->get_player_country()) {
				const portrait *interior_minister_portrait = this->get_owner()->get_game_data()->get_interior_minister_portrait();

				engine_interface::get()->add_notification(std::format("Landholder of {} Retired", this->get_current_cultural_name()), interior_minister_portrait, std::format("Your Excellency, after a distinguished career in our service, landholder {} of {} has decided to retire.", this->get_landholder()->get_full_name(), this->get_current_cultural_name()));
			}
		}

		this->set_landholder(nullptr);
		this->get_landholder()->get_game_data()->set_dead(true);
	}

	//if the site has no landholder, see if there is any character who can become its landholder
	if (this->get_landholder() == nullptr) {
		std::vector<const character *> potential_landholders;

		for (const character *character : this->site->get_landholders()) {
			assert_throw(character->has_role(character_role::landholder));

			const character_game_data *character_game_data = character->get_game_data();
			if (character_game_data->get_country() != nullptr) {
				continue;
			}

			if (character_game_data->is_dead()) {
				continue;
			}

			if (character->get_required_technology() != nullptr && !this->get_owner()->get_game_data()->has_technology(character->get_required_technology())) {
				continue;
			}

			if (character->get_obsolescence_technology() != nullptr && this->get_owner()->get_game_data()->has_technology(character->get_obsolescence_technology())) {
				continue;
			}

			if (character->get_conditions() != nullptr && !character->get_conditions()->check(this->get_owner(), read_only_context(this->get_owner()))) {
				continue;
			}

			potential_landholders.push_back(character);
		}

		if (!potential_landholders.empty()) {
			this->set_landholder(vector::get_random(potential_landholders));

			if (this->get_owner() == game::get()->get_player_country() && game::get()->is_running()) {
				const portrait *interior_minister_portrait = this->get_owner()->get_game_data()->get_interior_minister_portrait();

				engine_interface::get()->add_notification(std::format("New Landholder of {}", this->get_current_cultural_name()), interior_minister_portrait, std::format("{} has become the new landholder of {}!\n\n{}", this->get_landholder()->get_full_name(), this->get_current_cultural_name(), this->get_landholder()->get_game_data()->get_landholder_modifier_string(this->site)));
			}
		}
	}
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
		if (commodity->is_provincial() && this->get_province() != nullptr) {
			this->get_province()->get_game_data()->change_local_commodity_output(commodity, transportable_change);
		}
	} else {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_economy()->change_transportable_commodity_output(commodity, transportable_change);
		}
	}

	if (commodity->is_housing()) {
		this->change_housing(transportable_change);
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

	centesimal_int output_modifier = this->get_output_modifier();
	commodity_map<centesimal_int> commodity_output_modifiers = this->get_commodity_output_modifiers();

	if (this->get_resource() != nullptr && this->get_resource()->get_commodity() != nullptr) {
		commodity_output_modifiers[this->get_resource()->get_commodity()] += this->get_resource_output_modifier();
	}

	if (this->get_owner() != nullptr) {
		for (const auto &[commodity, value] : this->get_owner()->get_economy()->get_commodity_bonuses_per_population()) {
			outputs[commodity] += (value * this->get_population_unit_count());
		}

		if (this->is_capital()) {
			for (const auto &[commodity, value] : this->get_owner()->get_economy()->get_capital_commodity_bonuses()) {
				outputs[commodity] += value;
			}

			for (const auto &[commodity, value] : this->get_owner()->get_economy()->get_capital_commodity_bonuses_per_population()) {
				outputs[commodity] += (value * this->get_population_unit_count());
			}

			for (const auto &[commodity, modifier] : this->get_owner()->get_economy()->get_capital_commodity_output_modifiers()) {
				commodity_output_modifiers[commodity] += modifier;
			}
		}
	}

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

		if (this->get_resource() != nullptr && this->get_resource()->get_commodity() != nullptr) {
			commodity_output_modifiers[this->get_resource()->get_commodity()] += province->get_game_data()->get_resource_output_modifier();
		}

		for (const auto &[commodity, modifier] : province->get_game_data()->get_commodity_output_modifiers()) {
			commodity_output_modifiers[commodity] += modifier;
		}
	}

	if (this->get_owner() != nullptr) {
		output_modifier += this->get_owner()->get_economy()->get_output_modifier();

		if (this->get_resource() != nullptr && this->get_resource()->get_commodity() != nullptr) {
			commodity_output_modifiers[this->get_resource()->get_commodity()] += this->get_owner()->get_economy()->get_resource_output_modifier();
		}

		for (const auto &[commodity, modifier] : this->get_owner()->get_economy()->get_commodity_output_modifiers()) {
			commodity_output_modifiers[commodity] += modifier;
		}
	}

	for (auto &[commodity, output] : outputs) {
		const centesimal_int modifier = output_modifier + commodity_output_modifiers[commodity];

		if (modifier != 0) {
			output *= centesimal_int(100) + modifier;
			output /= 100;
		}

		if (commodity->is_labor() && (this->site->get_map_data()->get_type() == site_type::resource || this->site->get_map_data()->get_type() == site_type::celestial_body)) {
			//population units in resource sites do not produce labor
			output = centesimal_int(0);
		}

		this->set_commodity_output(commodity, output);
	}
}

void site_game_data::change_local_everyday_consumption(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	log_trace(std::format("Changing local everyday consumption in settlement {} of commodity {} (currently {}) by {}.", this->site->get_identifier(), commodity->get_identifier(), this->get_local_everyday_consumption(commodity).to_string(), change.to_string()));

	const centesimal_int count = (this->local_everyday_consumption[commodity] += change);

	log_trace(std::format("Changed local everyday consumption in settlement {} of commodity {} by {}, making it now {}.", this->site->get_identifier(), commodity->get_identifier(), change.to_string(), this->get_local_everyday_consumption(commodity).to_string()));

	assert_throw(count >= 0);

	if (count == 0) {
		this->local_everyday_consumption.erase(commodity);
	}
}

void site_game_data::change_local_luxury_consumption(const commodity *commodity, const centesimal_int &change)
{
	if (change == 0) {
		return;
	}

	log_trace(std::format("Changing local luxury consumption in settlement {} of commodity {} (currently {}) by {}.", this->site->get_identifier(), commodity->get_identifier(), this->get_local_luxury_consumption(commodity).to_string(), change.to_string()));

	const centesimal_int count = (this->local_luxury_consumption[commodity] += change);

	log_trace(std::format("Changed local luxury consumption in settlement {} of commodity {} by {}, making it now {}.", this->site->get_identifier(), commodity->get_identifier(), change.to_string(), this->get_local_luxury_consumption(commodity).to_string()));

	assert_throw(count >= 0);

	if (count == 0) {
		this->local_luxury_consumption.erase(commodity);
	}
}

std::vector<employment_location *> site_game_data::get_employment_locations()
{
	switch (this->site->get_map_data()->get_type()) {
		case site_type::settlement:
		case site_type::habitable_world:
		{
			std::vector<employment_location *> employment_locations;

			for (const qunique_ptr<settlement_building_slot> &building_slot : this->get_building_slots()) {
				if (building_slot->get_production_capacity() > 0) {
					employment_locations.push_back(building_slot.get());
				}
			}

			return employment_locations;
		}
		case site_type::resource:
		case site_type::celestial_body:
			if (this->get_resource() != nullptr && this->get_production_capacity() > 0) {
				return { this };
			}
			break;
		default:
			break;
	}

	return {};
}

void site_game_data::check_employment()
{
	const std::vector<employment_location *> employment_locations = vector::shuffled(this->get_employment_locations());

	for (employment_location *employment_location : employment_locations) {
		employment_location->check_superfluous_employment();
	}

	std::vector<population_unit *> unemployed_population_units;

	for (const qunique_ptr<population_unit> &population_unit : this->population_units) {
		if (population_unit->is_unemployed()) {
			unemployed_population_units.push_back(population_unit.get());
		}
	}

	vector::shuffle(unemployed_population_units);

	std::vector<employment_location *> food_employment_locations = employment_locations;
	std::erase_if(food_employment_locations, [this](const employment_location *employment_location) {
		for (const profession *profession : employment_location->get_employment_professions()) {
			if (profession->get_output_commodity()->is_food()) {
				return false;
			}
		}

		return true;
	});

	std::vector<employment_location *> non_food_employment_locations = employment_locations;
	std::erase_if(non_food_employment_locations, [this](const employment_location *employment_location) {
		for (const profession *profession : employment_location->get_employment_professions()) {
			if (profession->get_output_commodity()->is_food()) {
				return true;
			}
		}

		return false;
	});

	this->check_available_employment(food_employment_locations, unemployed_population_units);
	this->check_available_employment(non_food_employment_locations, unemployed_population_units);
}

void site_game_data::check_available_employment(const std::vector<employment_location *> &employment_locations, std::vector<population_unit *> &unemployed_population_units)
{
	if (unemployed_population_units.empty()) {
		return;
	}

	for (employment_location *employment_location : employment_locations) {
		centesimal_int available_production_capacity = employment_location->get_available_production_capacity();
		assert_throw(available_production_capacity >= 0);
		if (available_production_capacity == 0) {
			continue;
		}

		const std::vector<const profession *> professions = vector::shuffled(employment_location->get_employment_professions());
		for (const profession *profession : professions) {
			const commodity *output_commodity = profession->get_output_commodity();

			std::map<centesimal_int, std::vector<population_unit *>, std::greater<centesimal_int>> unemployed_population_units_by_output;
			for (population_unit *population_unit : unemployed_population_units) {
				const population_type *converted_population_type = nullptr;
				if (!employment_location->can_employ(population_unit, profession, converted_population_type)) {
					continue;
				}

				unemployed_population_units_by_output[employment_location->get_employee_commodity_outputs(converted_population_type ? converted_population_type : population_unit->get_type(), profession)[output_commodity]].push_back(population_unit);
			}

			for (const auto &[output, output_population_units] : unemployed_population_units_by_output) {
				for (population_unit *population_unit : output_population_units) {
					if (available_production_capacity < output) {
						break;
					}

					if (!employment_location->can_fulfill_inputs_for_employment(population_unit, profession)) {
						//if the inputs are not available, it is pointless to check other potential employees with the same output value
						break;
					}

					population_unit->set_employment_location(employment_location, profession, true);
					available_production_capacity -= output;
					std::erase(unemployed_population_units, population_unit);
				}

				if (available_production_capacity == 0) {
					break;
				}
			}

			employment_location->check_superfluous_employment();

			if (available_production_capacity == 0 || unemployed_population_units.empty()) {
				break;
			}
		}

		if (unemployed_population_units.empty()) {
			break;
		}
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
			if (commodity->is_provincial() && this->get_province() != nullptr) {
				this->get_province()->get_game_data()->change_local_commodity_output(commodity, transportable_change);
			}
		} else {
			if (this->get_owner() != nullptr) {
				this->get_owner()->get_economy()->change_transportable_commodity_output(commodity, transportable_change);
			}
		}
	}

	for (employment_location *employment_location : this->get_employment_locations()) {
		employment_location->check_excess_employment();
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
			if (commodity->is_provincial() && this->get_province() != nullptr) {
				this->get_province()->get_game_data()->change_local_commodity_output(commodity, transportable_change);
			}
		} else {
			if (this->get_owner() != nullptr) {
				this->get_owner()->get_economy()->change_transportable_commodity_output(commodity, transportable_change);
			}
		}
	}

	for (employment_location *employment_location : this->get_employment_locations()) {
		employment_location->check_excess_employment();
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
	const improvement *improvement = this->get_improvement(improvement_slot::main);
	return improvement != nullptr && improvement->is_visitable();
}

QVariantList site_game_data::get_visiting_armies_qvariant_list() const
{
	return container::to_qvariant_list(this->get_visiting_armies());
}

}
