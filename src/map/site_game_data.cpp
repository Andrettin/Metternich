#include "metternich.h"

#include "map/site_game_data.h"

#include "country/country.h"
#include "country/country_game_data.h"
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
#include "map/tile.h"
#include "population/population.h"
#include "population/population_type.h"
#include "population/population_unit.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/modifier.h"
#include "unit/military_unit.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/thread_pool.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

site_game_data::site_game_data(const metternich::site *site) : site(site)
{
	if (site->is_settlement()) {
		this->initialize_building_slots();
	}

	this->reset_non_map_data();
}

void site_game_data::reset_non_map_data()
{
	this->clear_buildings();
	this->clear_population_units();
	this->owner = nullptr;
	this->culture = nullptr;
	this->religion = nullptr;
	this->settlement_type = nullptr;
	this->housing = defines::get()->get_base_housing();
	this->free_food_consumption = site_game_data::base_free_food_consumption;
	this->score = this->site->is_settlement() ? site_game_data::base_settlement_score : 0;
	this->commodity_outputs.clear();
	this->base_commodity_outputs.clear();
	this->local_commodity_consumptions.clear();
	this->output_modifier = 0;
	this->commodity_output_modifiers.clear();
	this->visiting_military_units.clear();

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

		const int effective_consumption = std::min(consumption.to_int(), this->get_commodity_output(commodity));

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

void site_game_data::set_tile_pos(const QPoint &tile_pos)
{
	if (tile_pos == this->get_tile_pos()) {
		return;
	}

	if (this->get_province() != nullptr) {
		this->get_population()->remove_upper_population(this->get_province()->get_game_data()->get_population());
	}

	this->tile_pos = tile_pos;
	emit tile_pos_changed();

	if (this->get_province() != nullptr) {
		this->get_population()->add_upper_population(this->get_province()->get_game_data()->get_population());
	}
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
		this->population->remove_upper_population(old_owner->get_game_data()->get_population());
	}

	this->owner = owner;

	for (const qunique_ptr<population_unit> &population_unit : this->get_population_units()) {
		population_unit->set_country(owner);
	}

	if (this->get_owner() != nullptr) {
		this->population->add_upper_population(this->get_owner()->get_game_data()->get_population());
	}

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
			thread_pool::get()->co_spawn_sync([this]() -> boost::asio::awaitable<void> {
				co_await this->get_owner()->get_game_data()->create_diplomatic_map_mode_image(diplomatic_map_mode::cultural, {});
			});
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
			thread_pool::get()->co_spawn_sync([this]() -> boost::asio::awaitable<void> {
				co_await this->get_owner()->get_game_data()->create_diplomatic_map_mode_image(diplomatic_map_mode::religious, {});
			});
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
	const tile *tile = this->get_tile();
	if (tile != nullptr) {
		return tile->get_resource();
	}

	return nullptr;
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

bool site_game_data::has_building_class(const building_class *building_class) const
{
	const building_type *building = this->get_slot_building(building_class->get_slot_type());

	if (building == nullptr) {
		false;
	}

	return building->get_building_class() == building_class;
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

			if (building != this->get_culture()->get_building_class_type(building->get_building_class())) {
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

void site_game_data::on_settlement_built(const int multiplier)
{
	this->get_province()->get_game_data()->change_settlement_count(multiplier);

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_settlement_count(multiplier);
	}

	if (this->get_province() != nullptr && this->get_resource() != nullptr) {
		for (const auto &[commodity, value] : this->get_province()->get_game_data()->get_improved_resource_commodity_bonuses(this->get_resource())) {
			this->change_base_commodity_output(commodity, value * multiplier);
		}
	}
}

void site_game_data::on_building_gained(const building_type *building, const int multiplier)
{
	assert_throw(building != nullptr);
	assert_throw(multiplier != 0);
	assert_throw(this->get_province() != nullptr);

	this->change_score(building->get_score() * multiplier);

	if (this->get_owner() != nullptr) {
		country_game_data *country_game_data = this->get_owner()->get_game_data();
		country_game_data->change_settlement_building_count(building, multiplier);
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

	this->get_province()->get_game_data()->change_score(wonder->get_score() * multiplier);

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
		this->change_base_commodity_output(improvement->get_output_commodity(), improvement->get_output_multiplier() * multiplier);
	}

	if (this->get_province() != nullptr && this->get_resource() != nullptr) {
		for (const auto &[commodity, value] : this->get_province()->get_game_data()->get_improved_resource_commodity_bonuses(this->get_resource())) {
			this->change_base_commodity_output(commodity, value * multiplier);
		}
	}

	emit improvement_changed();
}

void site_game_data::change_score(const int change)
{
	if (change == 0) {
		return;
	}

	this->score += change;

	if (this->get_owner() != nullptr) {
		this->get_owner()->get_game_data()->change_score(change);
	}
}

void site_game_data::add_population_unit(qunique_ptr<population_unit> &&population_unit)
{
	this->get_population()->on_population_unit_gained(population_unit.get());

	this->population_units.push_back(std::move(population_unit));

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

	const country *owner = this->get_owner();
	if (owner != nullptr) {
		owner->get_game_data()->add_population_unit(population_unit.get());
	}

	this->add_population_unit(std::move(population_unit));
}

void site_game_data::on_population_type_count_changed(const population_type *type, const int change)
{
	if (type->get_output_commodity() != nullptr) {
		this->change_base_commodity_output(type->get_output_commodity(), type->get_output_value() * change);
	}

	for (const auto &[commodity, value] : type->get_consumed_commodities()) {
		if (commodity->is_local()) {
			this->change_local_commodity_consumption(commodity, value * change);
		}
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

void site_game_data::change_base_commodity_output(const commodity *commodity, const int change)
{
	if (change == 0) {
		return;
	}

	const int count = (this->base_commodity_outputs[commodity] += change);

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

	if (this->get_owner() != nullptr && !commodity->is_local()) {
		this->get_owner()->get_game_data()->change_commodity_output(commodity, output - old_output);
	}

	emit commodity_outputs_changed();
}

void site_game_data::calculate_commodity_outputs()
{
	commodity_map<int> outputs = this->base_commodity_outputs;

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
			output *= modifier;
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

}
