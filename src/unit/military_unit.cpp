#include "metternich.h"

#include "unit/military_unit.h"

#include "character/character.h"
#include "character/character_game_data.h"
#include "domain/country_military.h"
#include "domain/cultural_group.h"
#include "domain/culture.h"
#include "domain/diplomacy_state.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "game/game.h"
#include "infrastructure/improvement.h"
#include "language/name_generator.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/province_map_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "script/condition/and_condition.h"
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
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/vector_util.h"

namespace metternich {

military_unit::military_unit(const military_unit_type *type) : type(type)
{
	assert_throw(this->get_type() != nullptr);

	this->max_hit_points = type->get_stat(military_unit_stat::hit_points).to_int();
	this->set_hit_points(this->get_max_hit_points());
	this->set_morale(this->get_hit_points());

	for (const auto &[stat, value] : type->get_stats()) {
		this->set_stat(stat, value);
	}

	this->check_free_promotions();
}

military_unit::military_unit(const military_unit_type *type, const metternich::domain *domain, const metternich::phenotype *phenotype)
	: military_unit(type)
{
	this->domain = domain;
	this->phenotype = phenotype;

	this->generate_name();

	assert_throw(this->get_country() != nullptr);
	assert_throw(this->get_phenotype() != nullptr);

	connect(this, &military_unit::type_changed, this, &military_unit::icon_changed);

	this->get_country()->get_game_data()->change_military_score(this->get_score());

	for (int i = 0; i < static_cast<int>(military_unit_stat::count); ++i) {
		const military_unit_stat stat = static_cast<military_unit_stat>(i);
		const centesimal_int type_stat_value = type->get_stat_for_country(stat, this->get_country());
		this->change_stat(stat, type_stat_value - type->get_stat(stat));
	}

	this->check_free_promotions();
}

military_unit::military_unit(const military_unit_type *type, const metternich::domain *domain, const metternich::character *character)
	: military_unit(type, domain, character->get_phenotype())
{
	this->character = character;
	this->name = character->get_full_name();

	character->get_game_data()->set_military_unit(this);
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
			this->change_hit_points(std::min(this->get_hit_point_recovery_per_turn(), missing_hit_points));
		}

		const int missing_morale = this->get_hit_points() - this->get_morale();
		assert_throw(missing_morale >= 0);
		if (missing_morale > 0) {
			this->change_morale(std::min(this->get_morale_recovery_per_turn(), missing_morale));
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
	const std::map<std::string, int> &used_name_counts = this->get_country() ? this->get_country()->get_game_data()->get_unit_name_counts() : archimedes::map::empty_string_to_int_map;

	const culture_base *culture = this->get_culture();
	if (culture == nullptr) {
		culture = this->get_cultural_group();
	}

	if (culture == nullptr) {
		return;
	}

	this->name = culture->generate_military_unit_name(this->get_type(), used_name_counts);

	//if no name could be generated for the unit, give it a name along the patterns of "1st Regulars"
	int ordinal_name_count = 1;
	while (this->get_name().empty() && this->get_country() != nullptr) {
		std::string ordinal_name = std::format("{}{} {}", ordinal_name_count, number::get_ordinal_number_suffix(ordinal_name_count), this->get_type()->get_name());
		if (used_name_counts.contains(ordinal_name)) {
			++ordinal_name_count;
		} else {
			this->name = std::move(ordinal_name);
		}
	}

	if (!this->get_name().empty()) {
		log_trace(std::format("Generated name \"{}\" for military unit of type \"{}\" and culture \"{}\".", this->get_name(), this->get_type()->get_identifier(), culture->get_identifier()));
	}
}

void military_unit::set_type(const military_unit_type *type)
{
	if (type == this->get_type()) {
		return;
	}

	const military_unit_type *old_type = this->get_type();

	const bool different_category = this->get_category() != type->get_category();
	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), -1);
	}

	this->type = type;

	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), 1);
	}

	if (type->get_stat(military_unit_stat::hit_points).to_int() != old_type->get_stat(military_unit_stat::hit_points).to_int()) {
		this->change_max_hit_points(type->get_stat(military_unit_stat::hit_points).to_int() - old_type->get_stat(military_unit_stat::hit_points).to_int());
	}

	for (int i = 0; i < static_cast<int>(military_unit_stat::count); ++i) {
		const military_unit_stat stat = static_cast<military_unit_stat>(i);
		const centesimal_int type_stat_value = type->get_stat_for_country(stat, this->get_country());
		const centesimal_int old_type_stat_value = old_type->get_stat_for_country(stat, this->get_country());
		if (type_stat_value != old_type_stat_value) {
			this->change_stat(stat, type_stat_value - old_type_stat_value);
		}
	}

	//check promotions in case any have been invalidated by the type change, or if new free promotions have been gained
	this->check_promotions();

	emit type_changed();

	if (this->get_country() != nullptr && type->get_maintenance_commodity_costs() != old_type->get_maintenance_commodity_costs()) {
		emit this->get_country()->get_game_data()->maintenance_cost_changed();
	}
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

const metternich::culture *military_unit::get_culture() const
{
	if (this->get_country() != nullptr) {
		return this->get_country()->get_culture();
	}

	if (this->get_type()->get_culture() != nullptr) {
		return this->get_type()->get_culture();
	}

	return nullptr;
}

const metternich::cultural_group *military_unit::get_cultural_group() const
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

const metternich::religion *military_unit::get_religion() const
{
	if (this->get_country() != nullptr) {
		return this->get_country()->get_game_data()->get_religion();
	}

	return nullptr;
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

const metternich::character *military_unit::get_commander() const
{
	if (this->get_army() != nullptr) {
		return this->get_army()->get_commander();
	}

	return nullptr;
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
		case military_unit_domain::space:
			//air and space units can move both on land and water
			break;
		default:
			assert_throw(false);
	}

	if (province->is_water_zone()) {
		//water zones can be freely moved to, if there is a path to them, as they are never owned by countries
		return true;
	} else {
		const metternich::domain *province_owner = province->get_game_data()->get_owner();

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

bool military_unit::is_hostile_to(const metternich::domain *domain) const
{
	return this->get_country()->get_game_data()->can_attack(domain);
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

int military_unit::get_hit_point_recovery_per_turn() const
{
	return military_unit::hit_point_recovery_per_turn;
}

int military_unit::get_morale_recovery_per_turn() const
{
	return military_unit::morale_recovery_per_turn;
}

void military_unit::set_stat(const military_unit_stat stat, const centesimal_int &value)
{
	if (value == this->get_stat(stat)) {
		return;
	}

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(-this->get_score());
	}

	if (value == 0) {
		this->stats.erase(stat);
	} else {
		this->stats[stat] = value;
	}

	if (this->get_country() != nullptr) {
		this->get_country()->get_game_data()->change_military_score(this->get_score());
	}
}

centesimal_int military_unit::get_effective_stat(const military_unit_stat stat) const
{
	centesimal_int stat_value = this->get_stat(stat);

	const metternich::character *commander = this->get_commander();
	if (commander != nullptr) {
		stat_value += commander->get_game_data()->get_commanded_military_unit_stat_modifier(stat);
		stat_value += commander->get_game_data()->get_commanded_military_unit_type_stat_modifier(this->get_type(), stat);
	}

	return stat_value;
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

	this->promotions.push_back(promotion);

	if (promotion->get_modifier() != nullptr) {
		promotion->get_modifier()->apply(this);
	}

	if (game::get()->is_running()) {
		emit promotions_changed();
	}
}

void military_unit::remove_promotion(const promotion *promotion)
{
	std::erase(this->promotions, promotion);

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
			free_promotion_map = &this->get_country()->get_military()->get_free_infantry_promotion_counts();
		} else if (this->get_type()->is_cavalry()) {
			free_promotion_map = &this->get_country()->get_military()->get_free_cavalry_promotion_counts();
		} else if (this->get_type()->is_artillery()) {
			free_promotion_map = &this->get_country()->get_military()->get_free_artillery_promotion_counts();
		} else if (this->get_type()->is_ship()) {
			free_promotion_map = &this->get_country()->get_military()->get_free_warship_promotion_counts();
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

void military_unit::attack(military_unit *target, const bool ranged)
{
	assert_throw(target != nullptr);

	const metternich::province *province = target->get_province();
	const terrain_type *terrain = nullptr;
	if (province != nullptr && province->get_map_data()->get_terrain() != nullptr) {
		terrain = province->get_map_data()->get_terrain();
	}

	centesimal_int attack;
	if (ranged) {
		attack = this->get_effective_stat(military_unit_stat::missile);
	} else {
		attack = this->get_effective_stat(military_unit_stat::melee);
	}

	centesimal_int defense = target->get_effective_stat(military_unit_stat::defense);

	centesimal_int damage = attack * 2 - defense;
	damage /= 2;

	damage = centesimal_int::max(damage, 1);

	target->receive_damage(damage.to_int(), 0);
}

void military_unit::receive_damage(const int damage, const int morale_damage_modifier)
{
	this->change_hit_points(-damage);

	int morale_damage = damage;
	morale_damage *= 100 + morale_damage_modifier;
	morale_damage /= 100;

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
		this->get_country()->get_military()->remove_military_unit(this);
	}
}

void military_unit::disband()
{
	this->disband(false);
}

int military_unit::get_score() const
{
	int score = 0;

	for (const auto &[stat, stat_value] : this->stats) {
		if (is_percent_military_unit_stat(stat)) {
			score += stat_value.to_int() / 10;
		} else {
			score += (stat_value * 10).to_int();
		}
	}

	return score;
}

}
