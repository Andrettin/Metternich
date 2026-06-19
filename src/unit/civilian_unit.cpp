#include "metternich.h"

#include "unit/civilian_unit.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "culture/cultural_group.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "domain/domain_technology.h"
#include "economy/resource.h"
#include "engine_interface.h"
#include "game/game.h"
#include "infrastructure/building_slot.h"
#include "infrastructure/building_type.h"
#include "infrastructure/pathway.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "species/phenotype.h"
#include "unit/civilian_unit_type.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/random.h"
#include "util/string_conversion_util.h"
#include "util/vector_util.h"

namespace metternich {

civilian_unit::civilian_unit(const civilian_unit_type *type, const domain *owner, const metternich::phenotype *phenotype)
	: type(type), owner(owner), phenotype(phenotype)
{
	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_owner() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);

	this->generate_name();

	connect(this, &civilian_unit::type_changed, this, &civilian_unit::icon_changed);

	connect(this, &civilian_unit::province_changed, this, &civilian_unit::buildable_buildings_changed);
	connect(this, &civilian_unit::province_changed, this, &civilian_unit::buildable_pathway_changed);

	connect(this->get_owner()->get_game_data(), &domain_game_data::sites_changed, this, &civilian_unit::buildable_provinces_changed);
	connect(this->get_owner()->get_game_data(), &domain_game_data::settlement_building_counts_changed, this, &civilian_unit::buildable_provinces_changed);
	connect(this->get_owner()->get_technology(), &domain_technology::technologies_changed, this, &civilian_unit::buildable_provinces_changed);

	connect(this->get_owner()->get_game_data(), &domain_game_data::provinces_changed, this, &civilian_unit::improvable_resources_changed);
	connect(this->get_owner()->get_economy(), &domain_economy::commodity_outputs_changed, this, &civilian_unit::improvable_resources_changed);
	connect(this->get_owner()->get_technology(), &domain_technology::technologies_changed, this, &civilian_unit::improvable_resources_changed);

	connect(this->get_owner()->get_game_data(), &domain_game_data::provinces_changed, this, &civilian_unit::prospectable_provinces_changed);
	connect(this->get_owner()->get_game_data(), &domain_game_data::prospected_tiles_changed, this, &civilian_unit::prospectable_provinces_changed);
	connect(this->get_owner()->get_technology(), &domain_technology::technologies_changed, this, &civilian_unit::prospectable_provinces_changed);

	connect(this, &civilian_unit::original_province_changed, this, &civilian_unit::busy_changed);
	connect(this, &civilian_unit::work_progress_changed, this, &civilian_unit::busy_changed);
}

civilian_unit::civilian_unit(const civilian_unit_type *type, const domain *owner, const metternich::character *character)
	: civilian_unit(type, owner, character->get_phenotype())
{
	this->character = character;
	this->name = character->get_game_data()->get_full_name();

	character->get_game_data()->set_civilian_unit(this);
}

civilian_unit::civilian_unit(const gsml_data &scope)
{
	scope.process(this);

	assert_throw(this->get_type() != nullptr);
	assert_throw(this->get_owner() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
}

void civilian_unit::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "name") {
		this->name = value;
	} else if (key == "type") {
		this->type = civilian_unit_type::get(value);
	} else if (key == "owner") {
		this->owner = domain::get(value);
	} else if (key == "phenotype") {
		this->phenotype = phenotype::get(value);
	} else if (key == "character") {
		this->character = game::get()->get_character(value);
		this->character->get_game_data()->set_civilian_unit(this);
	} else if (key == "province") {
		this->set_province(province::get(value));
	} else if (key == "original_province") {
		this->original_province = province::get(value);
	} else if (key == "exploring") {
		this->exploring = string::to_bool(value);
	} else if (key == "prospecting") {
		this->prospecting = string::to_bool(value);
	} else if (key == "work_progress") {
		this->work_progress = decimillesimal_int(value);
	} else {
		throw std::runtime_error(std::format("Invalid civilian unit property: \"{}\".", key));
	}
}

void civilian_unit::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	throw std::runtime_error(std::format("Invalid civilian unit scope: \"{}\".", tag));
}

gsml_data civilian_unit::to_gsml_data() const
{
	gsml_data data;

	data.add_property("name", std::format("\"{}\"", this->get_name()));
	data.add_property("type", this->get_type()->get_identifier());
	data.add_property("owner", this->get_owner()->get_identifier());
	data.add_property("phenotype", this->get_phenotype()->get_identifier());

	if (this->get_character() != nullptr) {
		data.add_property("character", this->get_character()->get_identifier());
	}

	data.add_property("province", this->get_province()->get_identifier());

	if (this->original_province != nullptr) {
		data.add_property("original_province", this->original_province->get_identifier());
	}

	data.add_property("exploring", string::from_bool(this->exploring));
	data.add_property("prospecting", string::from_bool(this->prospecting));

	if (this->work_progress.has_value()) {
		data.add_property("work_progress", this->work_progress.value().to_string());
	}

	return data;
}

QCoro::Task<void> civilian_unit::do_turn()
{
	if (this->is_moving()) {
		this->set_original_province(nullptr);
	}

	if (this->work_progress.has_value()) {
		this->increment_work_progress();

		if (this->work_progress == 100) {
			this->set_work_progress(std::nullopt);

			if (this->exploring) {
				//set the adjacent provinces to explored for the owner player
				for (const metternich::province *neighbor_province : this->get_province()->get_map_data()->get_neighbor_provinces()) {
					if (!this->get_owner()->get_game_data()->is_province_explored(neighbor_province)) {
						co_await this->get_owner()->get_game_data()->explore_province(neighbor_province);
					}
				}

				this->exploring = false;
			}
			
			if (this->prospecting) {
				//co_await this->get_owner()->get_game_data()->prospect_province(this->get_province());
				this->prospecting = false;
			}
			
			if (this->under_construction_building != nullptr) {
				assert_throw(this->under_construction_building_site != nullptr);

				co_await this->under_construction_building_site->get_game_data()->add_building(this->under_construction_building);

				this->under_construction_building = nullptr;
				this->under_construction_building_site = nullptr;
			}

			if (this->under_construction_pathway != nullptr) {
				assert_throw(this->get_province() != nullptr);

				co_await this->get_province()->get_game_data()->set_pathway(this->under_construction_pathway);

				this->under_construction_pathway = nullptr;
			}
		}
	}
}

void civilian_unit::do_ai_turn()
{
	if (this->is_busy()) {
		return;
	}
}

void civilian_unit::generate_name()
{
	const std::map<std::string, int> &used_name_counts = this->get_owner() ? this->get_owner()->get_game_data()->get_unit_name_counts() : archimedes::map::empty_string_to_int_map;

	const culture_base *culture = this->get_culture();
	if (culture == nullptr) {
		culture = this->get_cultural_group();
	}

	if (culture == nullptr) {
		return;
	}

	this->name = culture->generate_given_name(gender::male, used_name_counts);

	if (!this->get_name().empty()) {
		log_trace(std::format("Generated name \"{}\" for civilian unit of type \"{}\" and culture \"{}\".", this->get_name(), this->get_type()->get_identifier(), culture->get_identifier()));
	}
}

const icon *civilian_unit::get_icon() const
{
	return this->get_type()->get_icon();
}

const metternich::culture *civilian_unit::get_culture() const
{
	if (this->get_owner() != nullptr) {
		return this->get_owner()->get_game_data()->get_culture();
	}

	if (this->get_type()->get_culture() != nullptr) {
		return this->get_type()->get_culture();
	}

	return nullptr;
}

const metternich::cultural_group *civilian_unit::get_cultural_group() const
{
	const culture *culture = this->get_culture();
	if (culture != nullptr) {
		return culture->get_group();
	}

	if (this->get_type()->get_cultural_group() != nullptr) {
		return this->get_type()->get_cultural_group();
	}

	return nullptr;
}

const province *civilian_unit::get_province() const
{
	return this->province;
}

void civilian_unit::set_province(const metternich::province *province)
{
	if (province == this->get_province()) {
		return;
	}

	if (this->get_province() != nullptr) {
		this->get_province()->get_game_data()->remove_civilian_unit(this);
	}

	this->province = province;

	if (this->get_province() != nullptr) {
		this->get_province()->get_game_data()->add_civilian_unit(this);
	}

	emit province_changed();
}

void civilian_unit::set_original_province(const metternich::province *province)
{
	if (province == this->original_province) {
		return;
	}

	const bool was_moving = this->is_moving();

	this->original_province = province;
	emit original_province_changed();

	if (this->get_owner() == game::get()->get_player_country()) {
		if (was_moving != this->is_moving()) {
			if (was_moving) {
				engine_interface::get()->add_active_civilian_unit(this);
			} else {
				engine_interface::get()->remove_active_civilian_unit(this);
			}
		}
	}
}

bool civilian_unit::can_move_to(const metternich::province *province) const
{
	if (province == this->get_province()) {
		return false;
	}

	if (province->get_game_data()->get_owner() == this->get_owner()) {
		return true;
	}

	if (province->get_game_data()->get_owner() != nullptr) {
		return province->get_game_data()->get_owner()->get_game_data()->is_any_vassal_of(this->get_owner());
	}

	return false;
}

void civilian_unit::move_to(const metternich::province *province)
{
	this->set_original_province(this->get_province());
	this->set_province(province);

	if (this->get_type()->is_explorer() && this->can_explore_province(province) && this->get_type()->is_prospector() && this->can_prospect_province(province)) {
		//explore and prospect at the same time
		this->exploring = true;
		this->prospecting = true;
		this->set_work_progress(decimillesimal_int(0));
		return;
	}

	if (this->get_type()->is_explorer() && this->can_explore_province(province)) {
		this->exploring = true;
		this->set_work_progress(decimillesimal_int(0));
		return;
	}

	if (this->get_type()->is_prospector() && this->can_prospect_province(province)) {
		this->prospecting = true;
		this->set_work_progress(decimillesimal_int(0));
		return;
	}

	if (this->can_build_on_tile()) {
		this->build_on_tile();
	}
}

void civilian_unit::cancel_move()
{
	assert_throw(this->original_province != nullptr);

	if (this->is_working()) {
		this->cancel_work();
	}

	this->set_province(this->original_province);
	this->set_original_province(nullptr);
}

std::vector<const building_type *> civilian_unit::get_buildable_buildings_for_site(const site *site) const
{
	assert_throw(site->is_settlement());
	assert_throw(site->get_game_data()->is_built());

	std::vector<const building_type *> holding_buildable_buildings;

	for (const qunique_ptr<building_slot> &building_slot : site->get_game_data()->get_building_slots()) {
		if (!building_slot->is_available()) {
			continue;
		}

		if (building_slot->get_under_construction_building() != nullptr) {
			continue;
		}

		const building_type *buildable_building = building_slot->get_buildable_building();
		if (buildable_building == nullptr) {
			continue;
		}

		if (!this->get_type()->can_build_building(buildable_building)) {
			continue;
		}

		holding_buildable_buildings.push_back(buildable_building);
	}

	return holding_buildable_buildings;
}

bool civilian_unit::has_buildable_buildings_for_site(const site *site) const
{
	assert_throw(site->is_settlement());
	assert_throw(site->get_game_data()->is_built());

	for (const qunique_ptr<building_slot> &building_slot : site->get_game_data()->get_building_slots()) {
		if (!building_slot->is_available()) {
			continue;
		}

		if (building_slot->get_under_construction_building() != nullptr) {
			continue;
		}

		const building_type *buildable_building = building_slot->get_buildable_building();
		if (buildable_building == nullptr) {
			continue;
		}

		if (!this->get_type()->can_build_building(buildable_building)) {
			continue;
		}

		return true;
	}

	return false;
}

site_map<std::vector<const building_type *>> civilian_unit::get_buildable_buildings() const
{
	if (this->get_province() == nullptr) {
		return {};
	}

	site_map<std::vector<const building_type *>> buildable_buildings;

	for (const site *site : this->get_province()->get_game_data()->get_settlement_sites()) {
		if (!site->get_game_data()->is_built()) {
			continue;
		}

		std::vector<const building_type *> holding_buildable_buildings = this->get_buildable_buildings_for_site(site);

		if (!holding_buildable_buildings.empty()) {
			buildable_buildings[site] = std::move(holding_buildable_buildings);
		}
	}

	return buildable_buildings;
}

QVariantList civilian_unit::get_buildable_buildings_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_buildable_buildings());
}

void civilian_unit::build_building(const building_type *building_type, const site *site)
{
	this->under_construction_building = building_type;
	this->under_construction_building_site = site;

	this->set_work_progress(decimillesimal_int(0));

	domain_economy *domain_economy = this->get_owner()->get_economy();

	for (const auto &[commodity, cost] : building_type->get_commodity_costs_for_site(site)) {
		domain_economy->change_stored_commodity(commodity, -cost);
	}
}

std::vector<const metternich::province *> civilian_unit::get_buildable_provinces() const
{
	std::vector<const metternich::province *> buildable_provinces;

	if (this->get_type()->get_buildable_buildings().empty()) {
		return buildable_provinces;
	}

	for (const site *site : this->get_owner()->get_game_data()->get_sites()) {
		if (!site->is_settlement() || !site->get_game_data()->is_built()) {
			continue;
		}

		if (vector::contains(buildable_provinces, site->get_game_data()->get_province())) {
			continue;
		}

		if (!this->has_buildable_buildings_for_site(site)) {
			continue;
		}

		buildable_provinces.push_back(site->get_game_data()->get_province());
	}

	return buildable_provinces;
}

QVariantList civilian_unit::get_buildable_provinces_qvariant_list() const
{
	return archimedes::container::to_qvariant_list(this->get_buildable_provinces());
}

const metternich::pathway *civilian_unit::get_buildable_pathway() const
{
	if (this->get_province() == nullptr) {
		return nullptr;
	}

	const pathway *buildable_pathway = this->get_province()->get_game_data()->get_buildable_pathway();
	if (buildable_pathway != nullptr && this->get_type()->can_build_pathway(buildable_pathway)) {
		return buildable_pathway;
	}

	return nullptr;
}

void civilian_unit::build_pathway(const metternich::pathway *pathway)
{
	this->under_construction_pathway = pathway;

	this->set_work_progress(decimillesimal_int(0));

	domain_economy *domain_economy = this->get_owner()->get_economy();

	for (const auto &[commodity, cost] : pathway->get_commodity_costs_for_province(this->get_province())) {
		domain_economy->change_stored_commodity(commodity, -cost);
	}
}

bool civilian_unit::can_build_on_tile() const
{
	return false;
}

void civilian_unit::build_on_tile()
{
}

void civilian_unit::cancel_work()
{
	if (this->under_construction_building != nullptr) {
		domain_economy *domain_economy = this->get_owner()->get_economy();

		for (const auto &[commodity, cost] : this->under_construction_building->get_commodity_costs_for_site(this->under_construction_building_site)) {
			domain_economy->change_stored_commodity(commodity, cost);
		}
	}

	if (this->under_construction_pathway != nullptr) {
		domain_economy *domain_economy = this->get_owner()->get_economy();

		for (const auto &[commodity, cost] : this->under_construction_pathway->get_commodity_costs_for_province(this->get_province())) {
			domain_economy->change_stored_commodity(commodity, cost);
		}
	}

	this->set_work_progress(std::nullopt);
	this->under_construction_building = nullptr;
	this->under_construction_building_site = nullptr;
	this->under_construction_pathway = nullptr;
	this->exploring = false;
	this->prospecting = false;
}

resource_map<std::vector<QPoint>> civilian_unit::get_improvable_resource_tiles() const
{
	resource_map<std::vector<QPoint>> resource_tiles;

	if (this->get_type()->get_improvable_resources().empty()) {
		return resource_tiles;
	}

	for (const metternich::province *province : this->get_owner()->get_game_data()->get_provinces()) {
		for (const QPoint &tile_pos : province->get_game_data()->get_resource_tiles()) {
			const tile *tile = map::get()->get_tile(tile_pos);

			if (tile->get_resource() == nullptr) {
				continue;
			}

			if (!this->get_type()->can_improve_resource(tile->get_resource())) {
				continue;
			}

			//const improvement *improvement = this->get_buildable_resource_improvement_for_tile(tile_pos);
			//if (improvement != nullptr) {
			//	resource_tiles[tile->get_resource()].push_back(tile_pos);
			//}
		}
	}

	return resource_tiles;
}

QVariantList civilian_unit::get_improvable_resource_tiles_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_improvable_resource_tiles());
}

bool civilian_unit::can_explore_province(const metternich::province *province) const
{
	if (!this->get_type()->is_explorer()) {
		return false;
	}

	if (!this->get_owner()->get_game_data()->is_province_explored(province)) {
		//can only explore already-explored provinces which border non-explored ones
		return false;
	}

	bool adjacent_unexplored = false;

	for (const metternich::province *neighbor_province : province->get_map_data()->get_neighbor_provinces()) {
		if (!this->get_owner()->get_game_data()->is_province_explored(neighbor_province)) {
			adjacent_unexplored = true;
			return true;
		}

		return false;
	}

	return adjacent_unexplored;
}

bool civilian_unit::can_prospect_province(const metternich::province *province) const
{
	if (!this->get_type()->is_prospector()) {
		return false;
	}

	for (const QPoint &tile_pos : province->get_map_data()->get_tiles()) {
		if (this->get_owner()->get_game_data()->is_tile_prospected(tile_pos)) {
			//already prospected
			continue;
		}

		const tile *tile = map::get()->get_tile(tile_pos);

		if (tile->is_resource_discovered()) {
			continue;
		}

		for (const resource *resource : resource::get_all()) {
			if (!resource->is_prospectable()) {
				continue;
			}

			if (!vector::contains(resource->get_terrain_types(), province->get_game_data()->get_terrain())) {
				continue;
			}

			if (resource->get_required_technology() != nullptr && !this->get_owner()->get_technology()->has_technology(resource->get_required_technology())) {
				continue;
			}

			return true;
		}
	}

	return false;
}

terrain_type_map<std::vector<const metternich::province *>> civilian_unit::get_prospectable_provinces() const
{
	terrain_type_map<std::vector<const metternich::province *>> prospectable_provinces;

	if (!this->get_type()->is_prospector()) {
		return prospectable_provinces;
	}

	terrain_type_set prospectable_terrains;

	for (const resource *resource : resource::get_all()) {
		if (!resource->is_prospectable()) {
			continue;
		}

		if (resource->get_required_technology() != nullptr && !this->get_owner()->get_technology()->has_technology(resource->get_required_technology())) {
			continue;
		}

		for (const terrain_type *resource_terrain : resource->get_terrain_types()) {
			prospectable_terrains.insert(resource_terrain);
		}
	}

	for (const metternich::province *province : this->get_owner()->get_game_data()->get_provinces()) {
		bool has_prospectable_terrain = false;
		for (const terrain_type *prospectable_terrain : prospectable_terrains) {
			if (province->get_map_data()->get_terrain() == prospectable_terrain) {
				has_prospectable_terrain = true;
				break;
			}
		}
		if (!has_prospectable_terrain) {
			continue;
		}

		if (!this->can_prospect_province(province)) {
			continue;
		}

		prospectable_provinces[province->get_game_data()->get_terrain()].push_back(province);
	}

	return prospectable_provinces;
}

QVariantList civilian_unit::get_prospectable_provinces_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_prospectable_provinces());
}

QString civilian_unit::get_work_progress_qstring() const
{
	if (!this->work_progress.has_value()) {
		return QString();
	}

	return QString::fromStdString(std::to_string(this->work_progress.value().to_int()));
}

void civilian_unit::set_work_progress(const std::optional<decimillesimal_int> &progress)
{
	if (progress == this->work_progress) {
		return;
	}

	if (progress > 100) {
		this->set_work_progress(decimillesimal_int(100));
		return;
	}

	const bool was_working = this->is_working();

	this->work_progress = progress;
	emit work_progress_changed();

	if (this->get_owner() == game::get()->get_player_country()) {
		if (was_working != this->is_working()) {
			if (was_working) {
				engine_interface::get()->add_active_civilian_unit(this);
			} else {
				engine_interface::get()->remove_active_civilian_unit(this);
			}
		}
	}
}

void civilian_unit::increment_work_progress()
{
	assert_throw(this->work_progress.has_value());

	const decimillesimal_int current_progress = this->work_progress.value();

	decimillesimal_int progress_change(100);

	if (this->exploring) {
		progress_change = decimillesimal_int::min(progress_change, decimillesimal_int(100) / civilian_unit::exploration_turns);
	}

	if (this->prospecting) {
		progress_change = decimillesimal_int::min(progress_change, decimillesimal_int(100) / civilian_unit::prospection_turns);
	}

	if (this->under_construction_building != nullptr) {
		if (this->under_construction_building->get_build_duration() != std::chrono::months(0)) {
			const int build_months = this->under_construction_building->get_build_duration().count();

			progress_change = decimillesimal_int::min(progress_change, decimillesimal_int(game::get()->get_current_months_per_turn()) * 100 / std::max(build_months, 1));
		} else {
			int64_t wealth_cost = 0;
			const commodity_map<int64_t> commodity_costs = this->under_construction_building->get_commodity_costs_for_site(this->under_construction_building_site);
			if (commodity_costs.contains(defines::get()->get_wealth_commodity())) {
				wealth_cost = commodity_costs.find(defines::get()->get_wealth_commodity())->second;
			}

			static constexpr dice progress_dice(1, 6);
			const int64_t wealth_cost_progress = random::get()->roll_dice(progress_dice) * defines::get()->get_domain_income_unit_value();

			progress_change = decimillesimal_int::min(progress_change, decimillesimal_int(wealth_cost_progress) * 100 / std::max(wealth_cost, 1ll));
		}
	}

	if (this->under_construction_pathway != nullptr) {
		int64_t wealth_cost = 0;
		const commodity_map<int64_t> commodity_costs = this->under_construction_pathway->get_commodity_costs_for_province(this->get_province());
		if (commodity_costs.contains(defines::get()->get_wealth_commodity())) {
			wealth_cost = commodity_costs.find(defines::get()->get_wealth_commodity())->second;
		}

		static constexpr dice progress_dice(1, 6);
		const int64_t wealth_cost_progress = random::get()->roll_dice(progress_dice) * defines::get()->get_domain_income_unit_value();

		progress_change = decimillesimal_int::min(progress_change, decimillesimal_int(wealth_cost_progress) * 100 / std::max(wealth_cost, 1ll));
	}

	this->set_work_progress(current_progress + progress_change);
}

QCoro::Task<void> civilian_unit::disband(const bool dead)
{
	if (this->get_character() != nullptr) {
		character_game_data *character_game_data = this->get_character()->get_game_data();
		character_game_data->set_civilian_unit(nullptr);

		if (dead) {
			co_await character_game_data->die();
		}
	}

	if (this->is_working()) {
		this->cancel_work();
	}

	assert_throw(this->get_province() != nullptr);
	this->set_province(nullptr);

	this->get_owner()->get_game_data()->remove_civilian_unit(this);
}

}
