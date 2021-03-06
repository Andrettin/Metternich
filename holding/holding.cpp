#include "holding/holding.h"

#include "character/character.h"
#include "culture/culture.h"
#include "culture/culture_group.h"
#include "culture/culture_supergroup.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/employment.h"
#include "economy/employment_type.h"
#include "game/engine_interface.h"
#include "game/game.h"
#include "history/history.h"
#include "holding/building.h"
#include "holding/building_slot.h"
#include "holding/holding_slot.h"
#include "holding/holding_slot_type.h"
#include "holding/holding_type.h"
#include "landed_title/landed_title.h"
#include "map/map.h"
#include "map/map_mode.h"
#include "map/province.h"
#include "map/terrain_type.h"
#include "politics/government_type.h"
#include "politics/government_type_group.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "religion/religion.h"
#include "religion/religion_group.h"
#include "script/holding_modifier.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/container_util.h"
#include "util/random.h"
#include "util/translator.h"
#include "util/vector_random_util.h"
#include "warfare/troop_type.h"

#include <utility>

namespace metternich {

holding::holding(metternich::holding_slot *slot, holding_type *type) : data_entry(slot->get_identifier()), slot(slot)
{
	this->change_base_population_growth(defines::get()->get_base_population_growth());
	this->set_type(type);

	if (this->get_barony() != nullptr) {
		this->set_owner(this->get_barony()->get_holder());
	}

	if (this->get_terrain() != nullptr && this->get_terrain()->get_holding_modifier() != nullptr) {
		this->get_terrain()->get_holding_modifier()->apply(this);
	}

	for (technology *technology : this->get_territory()->get_technologies()) {
		if (technology->get_holding_modifier() != nullptr) {
			technology->get_holding_modifier()->apply(this);
		}
	}

	this->change_base_population_capacity(this->get_territory()->get_population_capacity_additive_modifier());
	this->change_population_capacity_modifier(this->get_territory()->get_population_capacity_modifier());
	this->change_base_population_growth(this->get_territory()->get_population_growth_modifier());

	if (this->is_settlement()) {
		if (!slot->get_available_commodities().empty()) {
			metternich::commodity *commodity = vector::get_random(slot->get_available_commodities());
			this->set_commodity(commodity);
		} else {
			throw std::runtime_error("Holding slot \"" + slot->get_identifier() + "\" has no available commodities to produce.");
		}
	}

	connect(slot, &holding_slot::terrain_changed, this, &holding::terrain_changed);
	connect(slot, &holding_slot::active_trade_routes_changed, this, &holding::active_trade_routes_changed);
	connect(this, &holding::type_changed, this, &holding::titled_name_changed);
	connect(this, &holding::type_changed, this, &holding::portrait_path_changed);
	connect(this, &holding::culture_changed, this, &holding::portrait_path_changed);
	connect(this, &holding::religion_changed, this, &holding::portrait_path_changed);
}

holding::~holding()
{
	this->building_slots.clear();
}

void holding::initialize_history()
{
	if (this->get_owner() == nullptr && this->get_barony() == nullptr) {
		this->set_owner(this->get_territory()->get_county()->get_holder());
	}

	if (this->is_settlement()) {
		if (this->get_culture() == nullptr) {
			this->set_culture(this->get_territory()->get_culture());
		}

		if (this->get_religion() == nullptr) {
			this->set_religion(this->get_territory()->get_religion());
		}

		for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
			if (!population_unit->is_history_initialized()) {
				population_unit->initialize_history();
			}
		}

		this->remove_empty_population_units();
		this->sort_population_units();
		this->calculate_population();
		this->calculate_population_groups();
		this->check_overpopulation();
	}

	for (const auto &kv_pair : this->building_slots) {
		const qunique_ptr<building_slot> &building_slot = kv_pair.second;
		building_slot->initialize_history();
	}

	data_entry_base::initialize_history();
}

void holding::check_history() const
{
	if (this->is_settlement()) {
		if (this->get_culture() == nullptr) {
			throw std::runtime_error("The settlement holding of slot \"" + this->get_slot()->get_identifier() + "\" has no culture.");
		}

		if (this->get_religion() == nullptr) {
			throw std::runtime_error("The settlement holding of slot \"" + this->get_slot()->get_identifier() + "\" has no religion.");
		}
	}

	this->get_portrait_path(); //throws an exception if the portrait is not found

	try {
		for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
			population_unit->check_history();
		}
	} catch (...) {
		std::throw_with_nested(std::runtime_error("A population unit in the holding of slot \"" + this->get_slot()->get_identifier() + "\" is in an invalid state."));
	}
}

void holding::do_day()
{
	//handle construction
	if (this->get_under_construction_building() != nullptr) {
		this->change_construction_days(-1);
		if (this->get_construction_days() <= 0) {
			this->add_building(this->get_under_construction_building());
			this->set_under_construction_building(nullptr);
		}
	}

	for (employment *employment : this->get_employments()) {
		employment->do_day();
	}
}

void holding::do_month()
{
	if (this->is_settlement()) {
		this->do_population_growth();

		//use index-based for loop, as pop. units may add new ones in their do_month() function, e.g. due to mixing
		size_t pop_units_size = this->population_units.size();
		for (size_t i = 0; i < pop_units_size; ++i) {
			this->population_units[i]->do_month();
		}

		this->remove_empty_population_units();
		this->calculate_population_groups();
	}
}

void holding::do_year()
{
	this->sort_population_units();
}

std::string holding::get_name() const
{
	if (this->get_barony() != nullptr) {
		return translator::get()->translate(this->get_barony()->get_identifier_with_aliases(), this->get_tag_suffix_list_with_fallbacks());
	}

	return this->get_territory()->get_name();
}

std::string holding::get_type_name() const
{
	return translator::get()->translate(this->get_type()->get_identifier_with_aliases(), this->get_tag_suffix_list_with_fallbacks());
}

std::string holding::get_titled_name() const
{
	std::string titled_name = this->get_type_name() + " of ";
	titled_name += this->get_name();
	return titled_name;
}

std::vector<std::vector<std::string>> holding::get_tag_suffix_list_with_fallbacks() const
{
	std::vector<std::vector<std::string>> tag_list_with_fallbacks;

	if (this->get_barony() != nullptr) {
		if (this->get_barony()->get_government_type() != nullptr) {
			tag_list_with_fallbacks.push_back({this->get_barony()->get_government_type()->get_identifier(), government_type_group_to_string(this->get_barony()->get_government_type()->get_group())});
		}
	}

	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;

	if (this->is_settlement()) {
		culture = this->get_culture();
		religion = this->get_religion();
	} else {
		culture = this->get_territory()->get_culture();
		religion = this->get_territory()->get_religion();
	}

	tag_list_with_fallbacks.push_back({culture->get_identifier(), culture->get_group()->get_identifier(), culture->get_supergroup()->get_identifier()});
	tag_list_with_fallbacks.push_back({religion->get_identifier(), religion->get_group()->get_identifier()});

	if (this->get_province() != nullptr && this->get_province()->is_coastal()) {
		tag_list_with_fallbacks.push_back({"coastal"});
	}

	return tag_list_with_fallbacks;
}

landed_title *holding::get_barony() const
{
	return this->slot->get_barony();
}

void holding::set_type(holding_type *type)
{
	if (type == this->get_type()) {
		return;
	}

	if (this->get_type() != nullptr && this->get_type()->get_modifier() != nullptr) {
		this->get_type()->get_modifier()->remove(this);
	}

	this->type = type;

	if (this->get_type() != nullptr && this->get_type()->get_modifier() != nullptr) {
		this->get_type()->get_modifier()->apply(this);
	}

	emit type_changed();
	this->calculate_building_slots();
	this->update_population_types();
}

bool holding::is_settlement() const
{
	return this->get_slot()->is_settlement();
}

const std::filesystem::path &holding::get_portrait_path() const
{
	const std::string &base_tag = this->get_type()->get_portrait_tag();

	try {
		const std::filesystem::path &portrait_path = database::get()->get_tagged_holding_portrait_path(base_tag, this->get_tag_suffix_list_with_fallbacks());
		return portrait_path;
	} catch (...) {
		std::throw_with_nested(std::runtime_error("Could not find a valid portrait for holding of slot \"" + this->get_slot()->get_identifier() + "\"."));
	}
}

void holding::set_owner(character *new_owner)
{
	if (new_owner == this->get_owner()) {
		return;
	}

	character *old_owner = this->get_owner();
	if (old_owner != nullptr) {
		old_owner->remove_holding(this);
	}

	if (this->get_barony() != nullptr && this->get_barony()->get_holder() != new_owner) {
		throw std::runtime_error("Tried to set the owner of a holding which has a barony to a different character than its barony's holder.");
	} else if (is_extra_holding_slot_type(this->get_slot()->get_type()) && !is_separately_ownable_extra_holding_slot_type(this->get_slot()->get_type()) && this->get_territory()->get_owner() != new_owner) {
		throw std::runtime_error("Tried to set the owner of an extra holding which is not separately ownable to a different character than its territory's owner.");
	}

	this->owner = new_owner;
	emit owner_changed();

	if (new_owner != nullptr) {
		new_owner->add_holding(this);
	}

	if (this->get_province() != nullptr && this->get_slot()->get_type() == holding_slot_type::trading_post && map::get()->get_mode() == map_mode::trade_zone) {
		this->get_province()->update_color_for_map_mode(map::get()->get_mode());
	}
}

territory *holding::get_territory() const
{
	return this->get_slot()->get_territory();
}

province *holding::get_province() const
{
	return this->get_slot()->get_province();
}

world *holding::get_world() const
{
	return this->get_slot()->get_world();
}

landed_title *holding::get_realm() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_realm();
	}

	return nullptr;
}

const terrain_type *holding::get_terrain() const
{
	return this->get_slot()->get_terrain();
}

bool holding::is_territory_capital() const
{
	return this->get_slot()->is_territory_capital();
}

bool holding::can_have_population_type(const population_type *population_type) const
{
	return population_type->get_holding_types().contains(this->get_type());
}

population_type *holding::get_equivalent_population_type(const population_type *population_type) const
{
	//for the given population type, get an equivalent population type that is valid for the holding
	for (metternich::population_type *equivalent_type : population_type->get_equivalent_types()) {
		if (this->can_have_population_type(equivalent_type)) {
			return equivalent_type;
		}
	}

	return nullptr;
}

void holding::add_population_unit(qunique_ptr<population_unit> &&population_unit)
{
	this->change_population(population_unit->get_size());
	this->population_units.push_back(std::move(population_unit));
	emit population_units_changed();
}

/**
**	@brief	Get the population unit with a given type, culture, religion and phenotype, if any exist
**
**	@param	type		The population type
**	@param	culture		The culture
**	@param	religion	The religion
**	@param	phenotype	The phenotype
**
**	@return	The population unit
*/
population_unit *holding::get_population_unit(const population_type *type, const metternich::culture *culture, const metternich::religion *religion, const phenotype *phenotype) const
{
	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		if (population_unit->get_type() != type) {
			continue;
		}

		if (population_unit->get_culture() != culture) {
			continue;
		}

		if (population_unit->get_religion() != religion) {
			continue;
		}

		if (population_unit->get_phenotype() != phenotype) {
			continue;
		}

		return population_unit.get();
	}

	return nullptr;
}

/**
**	@brief	Change the population size of the holding's population unit with a given type, culture, religion and phenotype, creating a new one if no population unit is found with those characteristics and the change is positive
**
**	@param	type		The population type
**	@param	culture		The culture
**	@param	religion	The religion
**	@param	phenotype	The phenotype
**	@param	change		The population size change
*/
void holding::change_population_size(population_type *type, metternich::culture *culture, metternich::religion *religion, phenotype *phenotype, const int change)
{
	population_unit *population_unit = this->get_population_unit(type, culture, religion, phenotype);

	if (population_unit != nullptr) {
		population_unit->change_size(change);
	} else if (change > 0) {
		auto new_population_unit = make_qunique<metternich::population_unit>(type);
		new_population_unit->moveToThread(QApplication::instance()->thread());
		new_population_unit->set_holding(this);
		new_population_unit->set_size(change);
		new_population_unit->set_culture(culture);
		new_population_unit->set_religion(religion);
		new_population_unit->set_phenotype(phenotype);
		this->add_population_unit(std::move(new_population_unit));
	}
}

QVariantList holding::get_population_units_qvariant_list() const
{
	QVariantList list;

	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		list.append(QVariant::fromValue(population_unit.get()));
	}

	return list;
}

void holding::sort_population_units()
{
	std::sort(this->population_units.begin(), this->population_units.end(), [](const qunique_ptr<population_unit> &a, const qunique_ptr<population_unit> &b) {
		//give priority to population units with greater size, so that they will be displayed first
		return a->get_size() > b->get_size();
	});

	emit population_units_changed();
}

void holding::remove_empty_population_units()
{
	bool removed_pop_unit = false;
	for (size_t i = 0; i < this->population_units.size();) {
		const qunique_ptr<population_unit> &population_unit = this->population_units[i];
		if (population_unit->get_size() == 0) {
			this->population_units.erase(this->population_units.begin() + static_cast<int>(i));
			removed_pop_unit = true;
		} else {
			++i;
		}
	}

	if (removed_pop_unit) {
		emit population_units_changed();
	}
}

void holding::move_population_units_to(holding *other_holding)
{
	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		other_holding->change_population_size(population_unit->get_type(), population_unit->get_culture(), population_unit->get_religion(), population_unit->get_phenotype(), population_unit->get_size());
		population_unit->set_size(0);
	}

	this->remove_empty_population_units();
}

void holding::update_population_types()
{
	//convert the types of population units if they have become invalid to valid equivalent types
	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		if (!this->can_have_population_type(population_unit->get_type())) {
			population_type *equivalent_type = this->get_equivalent_population_type(population_unit->get_type());
			if (equivalent_type != nullptr) {
				population_unit->set_type(equivalent_type);
			}
		}
	}
}

void holding::set_population(const int population)
{
	if (population == this->get_population()) {
		return;
	}

	const int old_population = this->get_population();
	this->population = population;
	emit population_changed();

	this->calculate_population_growth(); //population growth depends on the current population

	//change the population count for the territory as well
	const int population_change = population - old_population;
	this->get_territory()->change_population(population_change);
}

void holding::calculate_population()
{
	int population = 0;
	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		population += population_unit->get_size();
	}
	this->set_population(population);
}

void holding::calculate_population_capacity()
{
	long long int population_capacity = this->get_base_population_capacity();
	population_capacity *= this->get_population_capacity_modifier();
	population_capacity /= 100;
	population_capacity *= this->get_holding_size();
	population_capacity /= holding_slot::default_holding_size;
	this->set_population_capacity(static_cast<int>(population_capacity));
}

void holding::do_population_growth()
{
	const int population_growth = this->get_population_growth();

	if (population_growth == 0) {
		return;
	}

	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		const int population_capacity_difference = this->get_population_capacity() - this->get_population();

		long long int change = static_cast<long long int>(population_unit->get_size()) * population_growth / 10000;
		if (change == 0) {
			if (population_growth != 0 && population_capacity_difference != 0) {
				//if the change is zero but population growth is non-zero, then make a change of 1
				if (population_growth > 0) {
					change = 1;
				} else {
					change = -1;
				}
			}
		} else if (change > 0 && change > population_capacity_difference) {
			change = population_capacity_difference; //don't grow the population beyond capacity
		}

		population_unit->change_size(static_cast<int>(change));
	}

	this->check_overpopulation();
}

/**
**	@brief	Check if the holding is overpopulated, and if so apply the "overpopulation" modifier
*/
void holding::check_overpopulation()
{
	//give the overpopulation modifier if the holding has become overpopulated, or remove it if it was overpopulated but isn't anymore
	const bool overpopulated = this->get_population() > this->get_population_capacity();
	if (overpopulated) {
		if (this->modifiers.find(holding_modifier::get_overpopulation_modifier()) == this->modifiers.end()) {
			holding_modifier::get_overpopulation_modifier()->apply(this);
			this->modifiers.insert(holding_modifier::get_overpopulation_modifier());
		}
	} else {
		if (this->modifiers.find(holding_modifier::get_overpopulation_modifier()) != this->modifiers.end()) {
			holding_modifier::get_overpopulation_modifier()->remove(this);
			this->modifiers.erase(holding_modifier::get_overpopulation_modifier());
		}
	}
}

void holding::calculate_population_groups()
{
	std::unique_lock<std::shared_mutex> lock(this->population_groups_mutex);

	this->population_per_type.clear();
	this->population_per_culture.clear();
	this->population_per_religion.clear();

	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		this->population_per_type[population_unit->get_type()] += population_unit->get_size();
		this->population_per_culture[population_unit->get_culture()] += population_unit->get_size();
		this->population_per_religion[population_unit->get_religion()] += population_unit->get_size();
	}

	emit population_groups_changed();

	//update the holding's main culture and religion

	metternich::culture *plurality_culture = nullptr;
	int plurality_culture_size = 0;

	for (const auto &kv_pair : this->population_per_culture) {
		metternich::culture *culture = kv_pair.first;
		const int culture_size = kv_pair.second;
		if (plurality_culture == nullptr || culture_size > plurality_culture_size) {
			plurality_culture = culture;
			plurality_culture_size = culture_size;
		}
	}

	metternich::religion *plurality_religion = nullptr;
	int plurality_religion_size = 0;

	for (const auto &kv_pair : this->population_per_religion) {
		metternich::religion *religion = kv_pair.first;
		const int religion_size = kv_pair.second;
		if (plurality_religion == nullptr || religion_size > plurality_religion_size) {
			plurality_religion = religion;
			plurality_religion_size = religion_size;
		}
	}

	this->set_culture(plurality_culture);
	this->set_religion(plurality_religion);
}

std::vector<building_slot *> holding::get_building_slots() const
{
	std::vector<building_slot *> building_slots;

	for (const auto &kv_pair : this->building_slots) {
		building_slots.push_back(kv_pair.second.get());
	}

	std::sort(building_slots.begin(), building_slots.end(), [](building_slot *a, building_slot *b) {
		//give priority to buildings that have already been built, so that they will be displayed first
		if (a->is_built() != b->is_built()) {
			return a->is_built();
		}

		if (a->is_buildable() != b->is_buildable()) {
			return a->is_buildable();
		}

		return a->get_building()->get_name() < b->get_building()->get_name();
	});

	return building_slots;
}

QVariantList holding::get_building_slots_qvariant_list() const
{
	return container::to_qvariant_list(this->get_building_slots());
}

std::set<building *> holding::get_buildings() const
{
	std::set<building *> buildings;

	for (const auto &kv_pair : this->building_slots) {
		if (kv_pair.second->is_built()) {
			buildings.insert(kv_pair.first);
		}
	}

	return buildings;
}

QVariantList holding::get_buildings_qvariant_list() const
{
	return container::to_qvariant_list(this->get_buildings());
}

bool holding::has_building(building *building) const
{
	building_slot *building_slot = this->get_building_slot(building);
	return building_slot != nullptr && building_slot->is_built();
}

void holding::add_building(building *building)
{
	if (history::get()->is_loading()) {
		//if is loading history, automatically add all required technologies to the territory when adding a building
		for (technology *required_technology : building->get_required_technologies()) {
			if (!this->get_territory()->has_technology(required_technology)) {
				this->get_territory()->add_technology(required_technology);
			}
		}
	}

	building_slot *building_slot = this->get_building_slot(building);
	building_slot->set_built(true);
}

void holding::remove_building(building *building)
{
	building_slot *building_slot = this->get_building_slot(building);
	building_slot->set_built(false);
}

void holding::calculate_building_slots()
{
	bool changed = false;

	std::set<building *> buildings_to_remove;
	for (const auto &kv_pair : this->building_slots) {
		if (!this->get_type()->has_building(kv_pair.first)) {
			buildings_to_remove.insert(kv_pair.first);
			changed = true;
		}
	}

	for (building *building : buildings_to_remove) {
		this->building_slots.erase(building);
	}

	std::set<building *> buildings_to_add;
	for (building *building : this->get_type()->get_buildings()) {
		if (!this->building_slots.contains(building)) {
			buildings_to_add.insert(building);
			changed = true;
		}
	}

	for (building *building : buildings_to_add) {
		auto new_building_slot = make_qunique<building_slot>(building, this);
		new_building_slot->moveToThread(QApplication::instance()->thread());

		connect(new_building_slot.get(), &building_slot::built_changed, this, &holding::buildings_changed);

		//to update the sorting of building slots
		connect(new_building_slot.get(), &building_slot::built_changed, this, &holding::building_slots_changed);

		if (!history::get()->is_loading()) {
			new_building_slot->create_condition_checks();
		}

		this->building_slots[building] = std::move(new_building_slot);
	}

	if (changed) {
		emit building_slots_changed();
	}
}

void holding::set_under_construction_building(building *building)
{
	if (building == this->get_under_construction_building()) {
		return;
	}

	this->under_construction_building = building;
	emit under_construction_building_changed();
	if (building != nullptr) {
		this->set_construction_days(building->get_construction_days());
	}
}

int holding::get_holding_size() const
{
	return this->get_slot()->get_holding_size();
}

bool holding::has_any_trade_route() const
{
	return this->get_slot()->has_any_trade_route();
}

bool holding::has_any_active_trade_route() const
{
	return this->get_slot()->has_any_active_trade_route();
}

bool holding::has_any_trade_route_land_connection() const
{
	return this->get_slot()->has_any_trade_route_land_connection();
}

QVariantList holding::get_levies_qvariant_list() const
{
	QVariantList levies;

	for (const auto &kv_pair : this->levies) {
		QVariantMap levy;
		levy["type"] = QVariant::fromValue(kv_pair.first);
		levy["size"] = QVariant::fromValue(kv_pair.second);
		levies.append(levy);
	}

	return levies;
}

QVariantList holding::get_troop_stats_qvariant_list() const
{
	QVariantList troop_stats;

	for (const auto &kv_pair : this->levies) {
		QVariantMap troop_stat;
		troop_type *troop_type = kv_pair.first;
		troop_stat["type"] = QVariant::fromValue(troop_type);
		troop_stat["attack"] = QVariant::fromValue(this->get_troop_attack(troop_type));
		troop_stat["defense"] = QVariant::fromValue(this->get_troop_defense(troop_type));
		troop_stats.append(troop_stat);
	}

	return troop_stats;
}

int holding::get_troop_attack(troop_type *troop_type) const
{
	return troop_type->get_attack() + this->get_troop_attack_modifier(troop_type);
}

int holding::get_troop_defense(troop_type *troop_type) const
{
	return troop_type->get_defense() + this->get_troop_defense_modifier(troop_type);
}

void holding::set_selected(const bool selected, const bool notify_engine_interface)
{
	if (selected == this->is_selected()) {
		return;
	}

	if (selected) {
		if (holding::selected_holding != nullptr) {
			holding::selected_holding->set_selected(false, false);
		}
		holding::selected_holding = this;
	} else {
		holding::selected_holding = nullptr;
	}

	this->selected = selected;
	emit selected_changed();

	if (notify_engine_interface) {
		engine_interface::get()->emit selected_holding_changed();
	}
}

QVariantList holding::get_population_per_type_qvariant_list() const
{
	std::shared_lock<std::shared_mutex> lock(this->population_groups_mutex);

	QVariantList population_per_type;

	for (const auto &kv_pair : this->population_per_type) {
		QVariantMap type_population;
		type_population["type"] = QVariant::fromValue(kv_pair.first);
		type_population["population"] = QVariant::fromValue(kv_pair.second);
		population_per_type.append(type_population);
	}

	return population_per_type;
}

QVariantList holding::get_population_per_culture_qvariant_list() const
{
	std::shared_lock<std::shared_mutex> lock(this->population_groups_mutex);

	QVariantList population_per_culture;

	for (const auto &kv_pair : this->population_per_culture) {
		QVariantMap culture_population;
		culture_population["culture"] = QVariant::fromValue(kv_pair.first);
		culture_population["population"] = QVariant::fromValue(kv_pair.second);
		population_per_culture.append(culture_population);
	}

	return population_per_culture;
}

QVariantList holding::get_population_per_religion_qvariant_list() const
{
	std::shared_lock<std::shared_mutex> lock(this->population_groups_mutex);

	QVariantList population_per_religion;

	for (const auto &kv_pair : this->population_per_religion) {
		QVariantMap religion_population;
		religion_population["religion"] = QVariant::fromValue(kv_pair.first);
		religion_population["population"] = QVariant::fromValue(kv_pair.second);
		population_per_religion.append(religion_population);
	}

	return population_per_religion;
}

void holding::order_construction(const QVariant &building_variant)
{
	QObject *building_object = qvariant_cast<QObject *>(building_variant);
	building *building = static_cast<metternich::building *>(building_object);
	game::get()->post_order([this, building]() {
		this->set_under_construction_building(building);
	});
}

}
