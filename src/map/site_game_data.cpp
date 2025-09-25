#include "metternich.h"

#include "map/site_game_data.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/party.h"
#include "database/defines.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "domain/country_economy.h"
#include "domain/country_government.h"
#include "domain/country_technology.h"
#include "domain/country_turn_data.h"
#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/domain_event.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/dungeon.h"
#include "infrastructure/dungeon_area.h"
#include "infrastructure/holding_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/improvement_slot.h"
#include "infrastructure/pathway.h"
#include "infrastructure/settlement_building_slot.h"
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

	connect(this, &site_game_data::holding_type_changed, this, &site_game_data::portrait_changed);
	connect(this, &site_game_data::dungeon_changed, this, &site_game_data::portrait_changed);
}

void site_game_data::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "holding_type") {
		this->holding_type = holding_type::get(value);
	} else if (key == "dungeon") {
		this->dungeon = dungeon::get(value);
	} else {
		throw std::runtime_error(std::format("Invalid site game data property: \"{}\".", key));
	}
}

void site_game_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "tile_pos") {
		const QPoint tile_pos = scope.to_point();
		this->site->get_map_data()->set_tile_pos(tile_pos);
		map::get()->set_tile_site(tile_pos, this->site);
	} else {
		throw std::runtime_error(std::format("Invalid site game data scope: \"{}\".", tag));
	}
}

gsml_data site_game_data::to_gsml_data() const
{
	gsml_data data(this->site->get_identifier());

	assert_throw(this->is_on_map());
	data.add_child("tile_pos", gsml_data::from_point(this->get_tile_pos()));

	if (this->get_holding_type() != nullptr) {
		data.add_property("holding_type", this->get_holding_type()->get_identifier());
	}

	if (this->get_dungeon() != nullptr) {
		data.add_property("dungeon", this->get_dungeon()->get_identifier());
	}

	return data;
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
	this->decrement_scripted_modifiers();
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

bool site_game_data::has_pathway(const pathway *pathway) const
{
	const tile *tile = this->get_tile();

	if (tile == nullptr) {
		return false;
	}

	return tile->has_pathway(pathway);
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

	return this->get_province()->get_game_data()->get_provincial_capital() == this->site;
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
	if (!this->site->is_settlement()) {
		return false;
	}

	if (!this->is_built()) {
		return false;
	}

	return true;
}

site_tier site_game_data::get_tier() const
{
	site_tier tier = site_tier::none;

	if (this->get_holding_type() != nullptr) {
		tier = static_cast<site_tier>(this->get_holding_type()->get_level());
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

void site_game_data::set_owner(const domain *owner)
{
	if (owner == this->get_owner()) {
		return;
	}

	const domain *old_owner = this->get_owner();

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

		old_owner->get_game_data()->remove_site(this->site);

		for (const auto &[commodity, output] : this->get_commodity_outputs()) {
			if (commodity->is_local()) {
				continue;
			}

			old_owner->get_economy()->change_commodity_output(commodity, -this->get_commodity_output(commodity));
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

			this->get_owner()->get_economy()->change_commodity_output(commodity, this->get_commodity_output(commodity));
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

		this->get_owner()->get_game_data()->add_site(this->site);
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

void site_game_data::set_holding_type(const metternich::holding_type *holding_type)
{
	assert_throw(this->site->is_settlement());

	if (holding_type == this->get_holding_type()) {
		return;
	}

	const metternich::holding_type *old_holding_type = this->get_holding_type();

	if (old_holding_type != nullptr) {
		if (old_holding_type->get_modifier() != nullptr) {
			old_holding_type->get_modifier()->apply(this->site, -1);
		}
	}

	this->holding_type = holding_type;

	if (old_holding_type == nullptr && this->get_holding_type() != nullptr) {
		this->on_settlement_built(1);

		if (this->get_owner() != nullptr) {
			if (this->get_owner()->get_game_data()->get_capital() == nullptr && this->can_be_capital()) {
				this->get_owner()->get_game_data()->set_capital(this->site);
			}
		}
	} else if (old_holding_type != nullptr && this->get_holding_type() == nullptr) {
		this->on_settlement_built(-1);

		if (this->get_owner() != nullptr) {
			if (this->get_owner()->get_game_data()->get_capital() == this->site) {
				this->get_owner()->get_game_data()->choose_capital();
			}
		}
	}

	if (this->get_holding_type() != nullptr) {
		assert_throw(this->get_dungeon() == nullptr);

		if (this->get_holding_type()->get_modifier() != nullptr) {
			this->get_holding_type()->get_modifier()->apply(this->site, 1);
		}

		this->check_building_conditions();
		this->check_free_buildings();
	}

	if (game::get()->is_running()) {
		emit holding_type_changed();
		emit map::get()->tile_holding_type_changed(this->get_tile_pos());
	}
}

void site_game_data::check_holding_type()
{
	if (this->get_holding_type() == nullptr) {
		return;
	}

	if (this->get_holding_type()->get_conditions() == nullptr || this->get_holding_type()->get_conditions()->check(this->site, read_only_context(this->site))) {
		return;
	}

	const std::vector<const metternich::holding_type *> potential_holding_types = this->get_best_holding_types(this->get_holding_type()->get_base_holding_types());

	assert_throw(!potential_holding_types.empty());
	this->set_holding_type(vector::get_random(potential_holding_types));
}

std::vector<const metternich::holding_type *> site_game_data::get_best_holding_types(const std::vector<const metternich::holding_type *> &holding_types) const
{
	std::vector<const metternich::holding_type *> potential_holding_types;

	int best_preserved_building_count = 0;

	for (const metternich::holding_type *base_holding_type : holding_types) {
		if (base_holding_type->get_conditions() != nullptr && !base_holding_type->get_conditions()->check(this->site, read_only_context(this->site))) {
			continue;
		}

		int preserved_building_count = 0;
		for (const qunique_ptr<settlement_building_slot> &building_slot : this->building_slots) {
			if (building_slot->get_building() == nullptr) {
				continue;
			}

			if (vector::contains(building_slot->get_building()->get_holding_types(), base_holding_type)) {
				++preserved_building_count;
			}
		}

		if (preserved_building_count < best_preserved_building_count) {
			continue;
		} else if (preserved_building_count > best_preserved_building_count) {
			potential_holding_types.clear();
			best_preserved_building_count = preserved_building_count;
		}

		potential_holding_types.push_back(base_holding_type);
	}

	if (potential_holding_types.empty()) {
		std::vector<const metternich::holding_type *> base_holding_types;

		for (const metternich::holding_type *holding_type : holding_types) {
			for (const metternich::holding_type *base_holding_type : holding_type->get_base_holding_types()) {
				if (!vector::contains(base_holding_types, base_holding_type)) {
					base_holding_types.push_back(base_holding_type);
				}
			}
		}

		return this->get_best_holding_types(base_holding_types);
	}

	return potential_holding_types;
}

bool site_game_data::is_built() const
{
	if (this->site->is_settlement()) {
		return this->get_holding_type() != nullptr;
	} else {
		return this->get_main_improvement() != nullptr && !this->get_main_improvement()->is_visitable();
	}
}

const resource *site_game_data::get_resource() const
{
	return this->site->get_map_data()->get_resource();
}

void site_game_data::set_dungeon(const metternich::dungeon *dungeon)
{
	if (dungeon == this->get_dungeon()) {
		return;
	}

	if (dungeon != nullptr) {
		assert_throw(this->get_holding_type() == nullptr);
		assert_throw(this->can_have_dungeon(dungeon));
	}

	this->dungeon = dungeon;

	this->explored_dungeon_areas.clear();

	if (game::get()->is_running()) {
		emit dungeon_changed();

		if (this->get_province() != nullptr) {
			emit this->get_province()->get_game_data()->visible_sites_changed();
		}
	}
}

bool site_game_data::can_have_dungeon(const metternich::dungeon *dungeon) const
{
	if (this->site->get_type() != site_type::dungeon && this->site->get_type() != site_type::holding) {
		return false;
	}

	if (this->get_holding_type() != nullptr) {
		return false;
	}

	if (this->get_dungeon() != nullptr && this->get_dungeon() != dungeon) {
		return false;
	}

	if (dungeon->get_conditions() != nullptr && !dungeon->get_conditions()->check(this->site, read_only_context(this->site))) {
		return false;
	}

	return true;
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

const portrait *site_game_data::get_portrait() const
{
	if (this->get_holding_type() != nullptr) {
		return this->get_holding_type()->get_portrait();
	}

	if (this->get_dungeon() != nullptr) {
		return this->get_dungeon()->get_portrait();
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

	const domain *owner = this->get_owner();
	if (owner == nullptr) {
		return;
	}

	if (this->get_culture() == nullptr) {
		return;
	}

	const domain_game_data *owner_game_data = owner->get_game_data();

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

	if (building->get_required_technology() != nullptr && (this->get_owner() == nullptr || !this->get_owner()->get_technology()->has_technology(building->get_required_technology()))) {
		return false;
	}

	building_slot->set_building(building);
	return true;
}

bool site_game_data::check_free_improvement(const improvement *improvement)
{
	if (improvement->get_required_technology() != nullptr && (this->get_owner() == nullptr || !this->get_owner()->get_technology()->has_technology(improvement->get_required_technology()))) {
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
		domain_game_data *domain_game_data = this->get_owner()->get_game_data();
		country_economy *country_economy = this->get_owner()->get_economy();
		domain_game_data->change_settlement_building_count(building, multiplier);

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
	return this->site->is_settlement() || this->site->get_type() == site_type::resource || (this->site->get_type() == site_type::celestial_body && this->get_resource() != nullptr);
}

bool site_game_data::can_have_population_type(const population_type *type) const
{
	assert_throw(this->can_have_population());
	assert_throw(this->is_built());

	if (this->site->is_settlement()) {
		return this->get_holding_type()->can_have_population_type(type);
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

	if (output == 0) {
		this->commodity_outputs.erase(commodity);
	} else {
		this->commodity_outputs[commodity] = output;
	}

	const centesimal_int output_change = output - old_output;

	if (commodity->is_local()) {
		if (commodity->is_provincial() && this->get_province() != nullptr) {
			this->get_province()->get_game_data()->change_local_commodity_output(commodity, output_change);
		}
	} else {
		if (this->get_owner() != nullptr) {
			this->get_owner()->get_economy()->change_commodity_output(commodity, output_change);
		}
	}

	if (commodity->is_housing()) {
		this->change_housing(output_change);
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

		this->set_commodity_output(commodity, output);
	}
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

void site_game_data::explore_dungeon(const std::shared_ptr<party> &party)
{
	assert_throw(!party->get_characters().empty());

	const std::vector<const dungeon_area *> potential_dungeon_areas = this->get_potential_dungeon_areas();

	if (potential_dungeon_areas.empty()) {
		//the dungeon has been fully explored

		if (party->get_domain() == game::get()->get_player_country()) {
			const portrait *war_minister_portrait = party->get_domain()->get_government()->get_war_minister_portrait();

			engine_interface::get()->add_notification("Dungeon Cleared", war_minister_portrait, std::format("You have cleared the {} dungeon!", this->get_dungeon()->get_name()));
		}

		this->set_dungeon(nullptr);
		return;
	}

	const dungeon_area *dungeon_area = vector::get_random(potential_dungeon_areas);

	context ctx(party->get_domain());
	ctx.root_scope = party->get_domain();
	ctx.party = party;
	ctx.dungeon_site = this->site;
	ctx.dungeon_area = dungeon_area;

	dungeon_area->get_event()->fire(party->get_domain(), ctx);
}

std::vector<const dungeon_area *> site_game_data::get_potential_dungeon_areas() const
{
	bool needs_entrance = true;
	for (const dungeon_area *dungeon_area : this->explored_dungeon_areas) {
		if (dungeon_area->is_entrance()) {
			needs_entrance = false;
			break;
		}
	}

	std::vector<const dungeon_area *> potential_dungeon_areas;

	for (const dungeon_area *dungeon_area : dungeon_area::get_all()) {
		if (dungeon_area->is_entrance() != needs_entrance) {
			continue;
		}

		if (this->get_explored_dungeon_areas().contains(dungeon_area)) {
			continue;
		}

		if (dungeon_area->get_conditions() != nullptr && !dungeon_area->get_conditions()->check(this->site, read_only_context(this->site))) {
			continue;
		}

		potential_dungeon_areas.push_back(dungeon_area);
	}

	return potential_dungeon_areas;
}

std::vector<const dungeon_area *> site_game_data::get_potential_dungeon_areas(const dungeon_area *additional_explored_area)
{
	if (additional_explored_area == nullptr || this->get_explored_dungeon_areas().contains(additional_explored_area)) {
		return this->get_potential_dungeon_areas();
	}

	assert_throw(!this->get_explored_dungeon_areas().contains(additional_explored_area));

	this->explored_dungeon_areas.insert(additional_explored_area);

	std::vector<const dungeon_area *> potential_dungeon_areas = this->get_potential_dungeon_areas();

	this->explored_dungeon_areas.erase(additional_explored_area);

	return potential_dungeon_areas;
}

const data_entry_set<dungeon_area> &site_game_data::get_explored_dungeon_areas() const
{
	return this->explored_dungeon_areas;
}

void site_game_data::add_explored_dungeon_area(const dungeon_area *dungeon_area)
{
	this->explored_dungeon_areas.insert(dungeon_area);
}

}
