#include "metternich.h"

#include "unit/military_unit.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "country/country.h"
#include "country/country_game_data.h"
#include "country/culture.h"
#include "country/diplomacy_state.h"
#include "country/religion.h"
#include "game/game.h"
#include "infrastructure/improvement.h"
#include "language/name_generator.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "script/condition/condition.h"
#include "script/modifier.h"
#include "ui/icon.h"
#include "unit/army.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "unit/promotion.h"
#include "unit/promotion_container.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/gender.h"
#include "util/log_util.h"
#include "util/set_util.h"
#include "util/vector_util.h"

namespace metternich {

military_unit::military_unit(const military_unit_type *type) : type(type)
{
	assert_throw(this->get_type() != nullptr);

	this->max_hit_points = type->get_hit_points();
	this->set_hit_points(this->get_max_hit_points());
	this->set_morale(this->get_hit_points());

	for (const auto &[stat, value] : type->get_stats()) {
		this->set_stat(stat, value);
	}

	this->check_free_promotions();
}

military_unit::military_unit(const military_unit_type *type, const metternich::country *country, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::site *home_settlement)
	: military_unit(type)
{
	this->country = country;
	this->culture = culture;
	this->religion = religion;
	this->phenotype = phenotype;
	this->home_settlement = home_settlement;

	this->generate_name();

	assert_throw(this->get_country() != nullptr);
	assert_throw(this->get_culture() != nullptr);
	assert_throw(this->get_religion() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);
	assert_throw(this->get_home_settlement() != nullptr);

	connect(this, &military_unit::type_changed, this, &military_unit::icon_changed);

	for (int i = 0; i < static_cast<int>(military_unit_stat::count); ++i) {
		const military_unit_stat stat = static_cast<military_unit_stat>(i);
		const centesimal_int type_stat_value = type->get_stat_for_country(stat, this->get_country());
		this->change_stat(stat, type_stat_value - type->get_stat(stat));
	}

	this->get_country()->get_game_data()->change_military_score(this->get_score());

	this->check_free_promotions();
}

military_unit::military_unit(const military_unit_type *type, const metternich::country *country, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::site *home_settlement)
	: military_unit(type, country, culture, religion, phenotype, home_settlement)
{
	this->population_type = population_type;

	assert_throw(this->get_population_type() != nullptr);
}

military_unit::military_unit(const military_unit_type *type, const metternich::character *character)
	: military_unit(type, character->get_game_data()->get_country(), character->get_culture(), character->get_religion(), character->get_phenotype(), character->get_home_settlement())
{
	this->character = character;
	this->name = character->get_full_name();

	character->get_game_data()->apply_military_unit_modifier(this, 1);

	//character military units do not have any province set as their home province, since they don't consume food
}

void military_unit::do_turn()
{
	if (!this->is_moving()) {
		const int missing_hit_points = this->get_max_hit_points() - this->get_hit_points();
		assert_throw(missing_hit_points >= 0);
		if (missing_hit_points > 0) {
			//recover unit HP if it is not moving
			this->change_hit_points(std::min(military_unit::hit_point_recovery_per_turn, missing_hit_points));
		}

		const int missing_morale = this->get_hit_points() - this->get_morale();
		assert_throw(missing_morale >= 0);
		if (missing_morale > 0) {
			this->change_morale(std::min(military_unit::morale_recovery_per_turn, missing_morale));
		}
	}
}

void military_unit::do_ai_turn()
{
	if (this->is_moving()) {
		return;
	}

	//FIXME: implement logic for upgrading military units, and for moving them to places in order to do combat or defend against attacks
}

void military_unit::generate_name()
{
	const std::set<std::string> &used_names = this->get_country() ? this->get_country()->get_game_data()->get_military_unit_names() : set::empty_string_set;

	this->name = this->get_culture()->generate_military_unit_name(this->get_type(), used_names);

	//if no name could be generated for the unit, give it a name along the patterns of "1st Regulars"
	int ordinal_name_count = 1;
	while (this->get_name().empty() && this->get_country() != nullptr) {
		std::string ordinal_name = std::format("{}{} {}", ordinal_name_count, number::get_ordinal_number_suffix(ordinal_name_count), this->get_type()->get_name());
		if (used_names.contains(ordinal_name)) {
			++ordinal_name_count;
		} else {
			this->name = std::move(ordinal_name);
		}
	}

	if (!this->get_name().empty()) {
		log_trace(std::format("Generated name \"{}\" for military unit of type \"{}\" and culture \"{}\".", this->get_name(), this->get_type()->get_identifier(), this->get_culture()->get_identifier()));
	}
}

void military_unit::set_type(const military_unit_type *type)
{
	if (type == this->get_type()) {
		return;
	}

	const military_unit_type *old_type = this->get_type();

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(-this->get_score());
	}

	const bool different_category = this->get_category() != type->get_category();
	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), -1);
	}

	this->type = type;

	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), 1);
	}

	if (type->get_hit_points() != old_type->get_hit_points()) {
		this->change_max_hit_points(type->get_hit_points() - old_type->get_hit_points());
	}

	for (int i = 0; i < static_cast<int>(military_unit_stat::count); ++i) {
		const military_unit_stat stat = static_cast<military_unit_stat>(i);
		const centesimal_int type_stat_value = type->get_stat_for_country(stat, this->get_country());
		const centesimal_int old_type_stat_value = old_type->get_stat_for_country(stat, this->get_country());
		if (type_stat_value != old_type_stat_value) {
			this->change_stat(stat, type_stat_value - old_type_stat_value);
		}
	}

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(this->get_score());
	}

	//check promotions in case any have been invalidated by the type change, or if new free promotions have been gained
	this->check_promotions();

	emit type_changed();
}

military_unit_category military_unit::get_category() const
{
	return this->get_type()->get_category();
}

military_unit_domain military_unit::get_domain() const
{
	return this->get_type()->get_domain();
}

const icon *military_unit::get_icon() const
{
	return this->get_type()->get_icon();
}

void military_unit::set_province(const metternich::province *province)
{
	if (province == this->get_province()) {
		return;
	}

	if (this->get_province() != nullptr) {
		this->get_province()->get_game_data()->remove_military_unit(this);
	}

	this->province = province;

	if (this->get_province() != nullptr) {
		this->get_province()->get_game_data()->add_military_unit(this);

		//when ships move to a water zone, explore all adjacent water zones and coasts as well
		if (this->get_province()->is_water_zone()) {
			for (const metternich::province *neighbor_province : this->get_province()->get_game_data()->get_neighbor_provinces()) {
				if (this->get_country()->get_game_data()->is_province_explored(neighbor_province)) {
					continue;
				}

				if (neighbor_province->is_water_zone()) {
					this->get_country()->get_game_data()->explore_province(neighbor_province);
				} else {
					//for coastal provinces bordering the water zone, explore all their tiles bordering it
					for (const QPoint &coastal_tile_pos : neighbor_province->get_game_data()->get_border_tiles()) {
						if (!map::get()->is_tile_on_province_border_with(coastal_tile_pos, this->get_province())) {
							continue;
						}

						if (!this->get_country()->get_game_data()->is_tile_explored(coastal_tile_pos)) {
							this->get_country()->get_game_data()->explore_tile(coastal_tile_pos);
						}
					}
				}
			}
		}
	}

	emit province_changed();
}

void military_unit::set_army(metternich::army *army)
{
	if (army == this->get_army()) {
		return;
	}

	const metternich::army *old_army = this->get_army();

	this->army = army;

	if (this->get_province() != nullptr) {
		if (army != nullptr && old_army == nullptr) {
			this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), -1);
		} else if (army == nullptr && old_army != nullptr) {
			this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), 1);
		}
	}

	if (game::get()->is_running()) {
		emit army_changed();
	}
}

bool military_unit::can_move_to(const metternich::province *province) const
{
	switch (this->get_domain()) {
		case military_unit_domain::land:
			if (province->is_water_zone()) {
				return false;
			}
			break;
		case military_unit_domain::water:
			if (!province->is_water_zone()) {
				//ships can only move from water to land provinces, but not between land provinces
				if (this->get_province() != nullptr && !this->get_province()->is_water_zone()) {
					return false;
				}
			}
			break;
		case military_unit_domain::air:
			//air units can move both on land and water
			break;
		default:
			assert_throw(false);
	}

	if (province->is_water_zone()) {
		//water zones can be freely moved to, if there is a path to them, as they are never owned by countries
		return true;
	} else {
		const metternich::country *province_owner = province->get_game_data()->get_owner();

		if (province_owner != nullptr) {
			if (province_owner == this->get_country()) {
				return true;
			}

			if (province_owner->get_game_data()->is_any_vassal_of(this->get_country())) {
				return true;
			}

			return this->get_country()->get_game_data()->can_attack(province_owner);
		}
	}

	return false;
}

bool military_unit::is_hostile_to(const metternich::country *country) const
{
	return this->get_country()->get_game_data()->can_attack(country);
}

void military_unit::set_hit_points(const int hit_points)
{
	if (hit_points == this->get_hit_points()) {
		return;
	}

	this->hit_points = hit_points;

	assert_throw(this->get_hit_points() <= this->get_max_hit_points());

	if (this->get_morale() > this->get_hit_points()) {
		this->set_morale(this->get_hit_points());
	}

	if (this->get_hit_points() <= 0) {
		this->disband(true);
	} else {
		emit hit_points_changed();
	}
}

int military_unit::get_discipline() const
{
	int discipline = this->get_stat(military_unit_stat::discipline).to_int();

	if (this->get_country() != nullptr) {
		const country_game_data *country_game_data = this->get_country()->get_game_data();

		switch (this->get_domain()) {
			case military_unit_domain::land:
				discipline += country_game_data->get_land_discipline_modifier();
				break;
			case military_unit_domain::water:
				discipline += country_game_data->get_naval_discipline_modifier();
				break;
			case military_unit_domain::air:
				discipline += country_game_data->get_air_discipline_modifier();
				break;
			default:
				break;
		}
	}

	//FIXME: add discipline from commander

	return discipline;
}

QVariantList military_unit::get_promotions_qvariant_list() const
{
	return container::to_qvariant_list(this->get_promotions());
}

bool military_unit::can_have_promotion(const promotion *promotion) const
{
	if (promotion->get_conditions() != nullptr && !promotion->get_conditions()->check(this, read_only_context(this))) {
		return false;
	}

	return true;
}

bool military_unit::has_promotion(const promotion *promotion) const
{
	return vector::contains(this->get_promotions(), promotion);
}

void military_unit::add_promotion(const promotion *promotion)
{
	if (vector::contains(this->get_promotions(), promotion)) {
		log::log_error(std::format("Tried to add promotion \"{}\" to military unit \"{}\" ({}), but it already has the promotion.", promotion->get_identifier(), this->get_name(), this->get_type()->get_name()));
		return;
	}

	const read_only_context ctx(this);

	if (promotion->get_conditions() != nullptr && !promotion->get_conditions()->check(this, ctx)) {
		log::log_error(std::format("Tried to add promotion \"{}\" to military unit \"{}\" ({}), for which the promotion's conditions are not fulfilled.", promotion->get_identifier(), this->get_name(), this->get_type()->get_name()));
		return;
	}

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(-this->get_score());
	}

	this->promotions.push_back(promotion);

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(this->get_score());
	}

	if (promotion->get_modifier() != nullptr) {
		promotion->get_modifier()->apply(this);
	}

	if (game::get()->is_running()) {
		emit promotions_changed();
	}
}

void military_unit::remove_promotion(const promotion *promotion)
{
	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(-this->get_score());
	}

	std::erase(this->promotions, promotion);

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(this->get_score());
	}

	if (promotion->get_modifier() != nullptr) {
		promotion->get_modifier()->remove(this);
	}

	if (game::get()->is_running()) {
		emit promotions_changed();
	}
}

void military_unit::check_promotions()
{
	this->check_free_promotions();

	std::vector<const promotion *> promotions_to_remove;

	const read_only_context ctx(this);

	for (const promotion *promotion : this->promotions) {
		if (vector::contains(promotions_to_remove, promotion)) {
			continue;
		}

		if (promotion->get_conditions() != nullptr && !promotion->get_conditions()->check(this, ctx)) {
			promotions_to_remove.push_back(promotion);
		}
	}

	if (!promotions_to_remove.empty()) {
		for (const promotion *promotion : promotions_to_remove) {
			this->remove_promotion(promotion);
		}

		//check promotions again, as the removal of a promotion might have invalidated other ones
		this->check_promotions();
	}
}

void military_unit::check_free_promotions()
{
	bool changed = false;

	for (const promotion *promotion : this->get_type()->get_free_promotions()) {
		if (this->has_promotion(promotion)) {
			continue;
		}

		if (!this->can_have_promotion(promotion)) {
			continue;
		}

		this->add_promotion(promotion);
		changed = true;
	}

	const promotion_map<int> *free_promotion_map = nullptr;
	if (this->get_country() != nullptr) {
		if (this->get_type()->is_infantry()) {
			free_promotion_map = &this->get_country()->get_game_data()->get_free_infantry_promotion_counts();
		} else if (this->get_type()->is_cavalry()) {
			free_promotion_map = &this->get_country()->get_game_data()->get_free_cavalry_promotion_counts();
		} else if (this->get_type()->is_artillery()) {
			free_promotion_map = &this->get_country()->get_game_data()->get_free_artillery_promotion_counts();
		} else if (this->get_type()->is_ship()) {
			free_promotion_map = &this->get_country()->get_game_data()->get_free_warship_promotion_counts();
		}
	}

	if (free_promotion_map != nullptr) {
		for (const auto &[promotion, count] : *free_promotion_map) {
			assert_throw(count > 0);

			if (this->has_promotion(promotion)) {
				continue;
			}

			if (!this->can_have_promotion(promotion)) {
				continue;
			}

			this->add_promotion(promotion);
			changed = true;
		}
	}

	if (changed) {
		//check free promotions again, as the addition of a free promotion might have caused the requirements of others to be fulfilled
		this->check_free_promotions();
	}
}

void military_unit::attack(military_unit *target, const bool ranged, const bool target_entrenched)
{
	assert_throw(target != nullptr);

	const metternich::province *province = target->get_province();
	const terrain_type *terrain = nullptr;
	if (province != nullptr) {
		terrain = province->get_provincial_capital()->get_game_data()->get_tile()->get_terrain();
	}

	centesimal_int attack;
	if (ranged) {
		attack = this->get_stat(military_unit_stat::firepower);
	} else {
		attack = this->get_stat(military_unit_stat::melee);
	}
	int attack_modifier = 0;
	attack_modifier += this->get_stat(military_unit_stat::damage_bonus).to_int();
	if (target->get_type()->is_infantry()) {
		attack_modifier += this->get_stat(military_unit_stat::bonus_vs_infantry).to_int();
	} else if (target->get_type()->is_cavalry()) {
		attack_modifier += this->get_stat(military_unit_stat::bonus_vs_cavalry).to_int();
	} else if (target->get_type()->is_artillery()) {
		attack_modifier += this->get_stat(military_unit_stat::bonus_vs_artillery).to_int();
	}
	if (terrain != nullptr) {
		if (terrain->is_desert()) {
			attack_modifier += this->get_stat(military_unit_stat::desert_attack_modifier).to_int();
		} else if (terrain->is_forest()) {
			attack_modifier += this->get_stat(military_unit_stat::forest_attack_modifier).to_int();
		} else if (terrain->is_hills()) {
			attack_modifier += this->get_stat(military_unit_stat::hills_attack_modifier).to_int();
		} else if (terrain->is_mountains()) {
			attack_modifier += this->get_stat(military_unit_stat::mountains_attack_modifier).to_int();
		} else if (terrain->is_wetland()) {
			attack_modifier += this->get_stat(military_unit_stat::wetland_attack_modifier).to_int();
		}
	}
	if (attack_modifier != 0) {
		attack *= 100 + attack_modifier;
		attack /= 100;
	}

	centesimal_int defense = target->get_stat(military_unit_stat::defense);
	int defense_modifier = 0;
	if (ranged) {
		defense_modifier += target->get_stat(military_unit_stat::ranged_defense_modifier).to_int();
	}
	if (terrain != nullptr) {
		if (terrain->is_desert()) {
			defense_modifier += target->get_stat(military_unit_stat::desert_defense_modifier).to_int();
		} else if (terrain->is_forest()) {
			defense_modifier += target->get_stat(military_unit_stat::forest_defense_modifier).to_int();
		} else if (terrain->is_hills()) {
			defense_modifier += target->get_stat(military_unit_stat::hills_defense_modifier).to_int();
		} else if (terrain->is_mountains()) {
			defense_modifier += target->get_stat(military_unit_stat::mountains_defense_modifier).to_int();
		} else if (terrain->is_wetland()) {
			defense_modifier += target->get_stat(military_unit_stat::wetland_defense_modifier).to_int();
		}
	}
	if (defense_modifier != 0) {
		defense *= 100 + defense_modifier;
		defense /= 100;
	}
	if (target_entrenched) {
		int entrenchment_bonus = target->get_type()->get_entrenchment_bonus();
		entrenchment_bonus *= 100 + target->get_stat(military_unit_stat::entrenchment_bonus_modifier).to_int();
		entrenchment_bonus /= 100;
		defense += entrenchment_bonus;
	}

	centesimal_int damage = attack * 2 - defense;
	damage /= 2;

	damage *= 100 + target->get_stat(military_unit_stat::resistance).to_int();
	damage /= 100;

	damage = centesimal_int::max(damage, 1);

	target->receive_damage(damage.to_int(), this->get_stat(military_unit_stat::shock).to_int());
}

void military_unit::receive_damage(const int damage, const int morale_damage_modifier)
{
	this->change_hit_points(-damage);

	int morale_damage = damage;
	morale_damage *= 100 + morale_damage_modifier;
	morale_damage /= 100 + this->get_discipline();

	this->change_morale(-morale_damage);
}

void military_unit::heal(const int healing)
{
	const int missing_hit_points = this->get_max_hit_points() - this->get_hit_points();

	if (missing_hit_points == 0) {
		return;
	}

	this->change_hit_points(std::min(healing, missing_hit_points));
}

void military_unit::disband(const bool dead)
{
	if (this->get_character() != nullptr) {
		character_game_data *character_game_data = this->get_character()->get_game_data();
		character_game_data->set_military_unit(nullptr);

		if (character_game_data->get_country() != nullptr) {
			character_game_data->get_country()->get_game_data()->remove_leader(this->get_character());
		}

		if (dead) {
			character_game_data->set_dead(true);
		}
	}

	if (this->get_army() != nullptr) {
		this->army->remove_military_unit(this);
	}

	if (this->get_province() != nullptr) {
		this->get_province()->get_game_data()->remove_military_unit(this);
	}

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(-this->get_score());
		this->get_country()->get_game_data()->remove_military_unit(this);

		if (!dead) {
			assert_throw(this->get_population_type() != nullptr);
			assert_throw(this->get_culture() != nullptr);
			assert_throw(this->get_religion() != nullptr);
			assert_throw(this->get_phenotype() != nullptr);
			assert_throw(this->get_home_settlement() != nullptr);

			this->get_home_settlement()->get_game_data()->create_population_unit(this->get_population_type(), this->get_culture(), this->get_religion(), this->get_phenotype());
		}
	}
}

void military_unit::disband()
{
	this->disband(false);
}

int military_unit::get_score() const
{
	return this->get_type()->get_score() + static_cast<int>(this->get_promotions().size()) * 100;
}

}
