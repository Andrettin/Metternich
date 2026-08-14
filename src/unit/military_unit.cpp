#include "metternich.h"

#include "unit/military_unit.h"

#include "character/character.h"
#include "character/character_defines.h"
#include "character/character_game_data.h"
#include "culture/cultural_group.h"
#include "culture/culture.h"
#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_diplomacy.h"
#include "domain/domain_economy.h"
#include "domain/domain_game_data.h"
#include "domain/domain_military.h"
#include "economy/commodity.h"
#include "game/attack_result.h"
#include "game/battle_resolution_table.h"
#include "game/battle_resolution_type.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/and_condition.h"
#include "script/modifier.h"
#include "ui/icon.h"
#include "unit/army.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_stat.h"
#include "unit/military_unit_type.h"
#include "unit/promotion.h"
#include "unit/promotion_container.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/log_util.h"
#include "util/map_util.h"
#include "util/number_util.h"
#include "util/vector_random_util.h"
#include "util/vector_util.h"

namespace metternich {

QCoro::Task<qunique_ptr<military_unit>> military_unit::create(const military_unit_type *type)
{
	auto military_unit = make_qunique<metternich::military_unit>(type);
	co_await military_unit->set_hit_points(military_unit->get_max_hit_points());
	co_await military_unit->check_free_promotions();
	co_return military_unit;
}

QCoro::Task<qunique_ptr<military_unit>> military_unit::create(const military_unit_type *type, const metternich::domain *domain, const metternich::phenotype *phenotype)
{
	auto military_unit = co_await metternich::military_unit::create(type);

	military_unit->domain = domain;
	military_unit->phenotype = phenotype;

	military_unit->generate_name();

	assert_throw(military_unit->get_country() != nullptr);
	assert_throw(military_unit->get_phenotype() != nullptr);

	military_unit->get_country()->get_game_data()->change_military_score(military_unit->get_score());

	for (int i = 0; i < static_cast<int>(military_unit_stat::count); ++i) {
		const military_unit_stat stat = static_cast<military_unit_stat>(i);
		const centesimal_int type_stat_value = type->get_stat_for_domain(stat, military_unit->get_country());
		military_unit->change_stat(stat, type_stat_value - type->get_stat(stat));
	}

	co_await military_unit->check_free_promotions();

	co_return military_unit;
}

QCoro::Task<qunique_ptr<military_unit>> military_unit::create(const military_unit_type *type, const metternich::domain *domain, const metternich::character *character)
{
	auto military_unit = co_await metternich::military_unit::create(type, domain, character->get_phenotype());

	military_unit->character = character;
	military_unit->name = character->get_game_data()->get_full_name();

	co_await character->get_game_data()->set_military_unit(military_unit.get());
	co_await character->get_game_data()->apply_military_unit_modifier(military_unit.get(), 1);

	//character military units do not have any province set as their home province, since they don't consume food

	co_return military_unit;
}


military_unit::military_unit(const military_unit_type *type) : type(type)
{
	assert_throw(this->get_type() != nullptr);

	this->max_hit_points = type->get_stat(military_unit_stat::hit_points).to_int();

	for (const auto &[stat, value] : type->get_stats()) {
		this->set_stat(stat, value);
	}

	if (!type->get_battle_resolution_types().empty()) {
		this->battle_resolution_type = vector::get_random(type->get_battle_resolution_types());
	} else {
		this->battle_resolution_type = static_cast<metternich::battle_resolution_type>(random::get()->generate_in_range(1, static_cast<int>(battle_resolution_type::count) - 1));
	}

	connect(this, &military_unit::type_changed, this, &military_unit::icon_changed);
}

QCoro::Task<void> military_unit::do_turn()
{
	if (!this->is_moving()) {
		const int missing_hit_points = this->get_max_hit_points() - this->get_hit_points();
		assert_throw(missing_hit_points >= 0);
		if (missing_hit_points > 0) {
			//recover unit HP if it is not moving
			co_await this->change_hit_points(std::min(this->get_hit_point_recovery_per_turn(), missing_hit_points));
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

QCoro::Task<void> military_unit::set_type(const military_unit_type *type)
{
	if (type == this->get_type()) {
		co_return;
	}

	const military_unit_type *old_type = this->get_type();

	const bool different_category = this->get_category() != type->get_category();
	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), -1);
	}

	for (const auto &[commodity, cost] : old_type->get_commodity_costs()) {
		if (commodity->is_manpower()) {
			co_await this->get_country()->get_economy()->change_commodity_storage_capacity(commodity, cost);
		}
	}

	this->type = type;

	if (this->get_province() != nullptr && different_category) {
		this->get_province()->get_game_data()->change_military_unit_category_count(this->get_category(), 1);
	}

	if (type->get_stat(military_unit_stat::hit_points).to_int() != old_type->get_stat(military_unit_stat::hit_points).to_int()) {
		co_await this->change_max_hit_points(type->get_stat(military_unit_stat::hit_points).to_int() - old_type->get_stat(military_unit_stat::hit_points).to_int());
	}

	for (int i = 0; i < static_cast<int>(military_unit_stat::count); ++i) {
		const military_unit_stat stat = static_cast<military_unit_stat>(i);
		const centesimal_int type_stat_value = type->get_stat_for_domain(stat, this->get_country());
		const centesimal_int old_type_stat_value = old_type->get_stat_for_domain(stat, this->get_country());
		if (type_stat_value != old_type_stat_value) {
			this->change_stat(stat, type_stat_value - old_type_stat_value);
		}
	}

	if (this->get_battle_resolution_type() == battle_resolution_type::none || !vector::contains(type->get_battle_resolution_types(), this->get_battle_resolution_type())) {
		if (!type->get_battle_resolution_types().empty()) {
			this->battle_resolution_type = vector::get_random(type->get_battle_resolution_types());
		} else {
			this->battle_resolution_type = static_cast<metternich::battle_resolution_type>(random::get()->generate_in_range(1, static_cast<int>(battle_resolution_type::count) - 1));
		}
	}

	for (const auto &[commodity, cost] : type->get_commodity_costs()) {
		if (commodity->is_manpower()) {
			co_await this->get_country()->get_economy()->change_commodity_storage_capacity(commodity, -cost);
		}
	}

	//check promotions in case any have been invalidated by the type change, or if new free promotions have been gained
	co_await this->check_promotions();

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
		return this->get_country()->get_game_data()->get_culture();
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

QCoro::Task<void> military_unit::set_province(const metternich::province *province)
{
	if (province == this->get_province()) {
		co_return;
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

				co_await this->get_country()->get_game_data()->explore_province(neighbor_province);
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

			if (province_owner->get_diplomacy()->is_any_vassal_of(this->get_country())) {
				return true;
			}

			return this->get_country()->get_diplomacy()->can_attack(province_owner);
		}
	}

	return false;
}

bool military_unit::is_hostile_to(const metternich::domain *domain) const
{
	return this->get_country()->get_diplomacy()->can_attack(domain);
}

QCoro::Task<void> military_unit::set_hit_points(const int hit_points)
{
	if (hit_points == this->get_hit_points()) {
		co_return;
	}

	this->hit_points = hit_points;

	assert_throw(this->get_hit_points() <= this->get_max_hit_points());

	if (this->get_hit_points() <= 0) {
		co_await this->disband(true);
	} else {
		emit hit_points_changed();
	}
}

int military_unit::get_hit_point_recovery_per_turn() const
{
	return military_unit::hit_point_recovery_per_turn;
}

QCoro::Task<void> military_unit::fully_recover()
{
	co_await this->set_hit_points(this->get_max_hit_points());

	if (this->get_character() != nullptr) {
		co_await this->get_character()->get_game_data()->fully_recover();
	}
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

	emit stats_changed();
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

int military_unit::get_battle_movement() const
{
	return (this->get_effective_stat(military_unit_stat::movement) * defines::get()->get_battle_map_scale()).to_int();
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

QCoro::Task<void> military_unit::add_promotion(const promotion *promotion)
{
	if (vector::contains(this->get_promotions(), promotion)) {
		log::log_error(std::format("Tried to add promotion \"{}\" to military unit \"{}\" ({}), but it already has the promotion.", promotion->get_identifier(), this->get_name(), this->get_type()->get_name()));
		co_return;
	}

	const read_only_context ctx(this);
	if (promotion->get_conditions() != nullptr && !promotion->get_conditions()->check(this, ctx)) {
		log::log_error(std::format("Tried to add promotion \"{}\" to military unit \"{}\" ({}), for which the promotion's conditions are not fulfilled.", promotion->get_identifier(), this->get_name(), this->get_type()->get_name()));
		co_return;
	}

	this->promotions.push_back(promotion);

	if (promotion->get_modifier() != nullptr) {
		co_await promotion->get_modifier()->apply(this);
	}

	if (game::get()->is_running()) {
		emit promotions_changed();
	}
}

QCoro::Task<void> military_unit::remove_promotion(const promotion *promotion)
{
	std::erase(this->promotions, promotion);

	if (promotion->get_modifier() != nullptr) {
		co_await promotion->get_modifier()->remove(this);
	}

	if (game::get()->is_running()) {
		emit promotions_changed();
	}
}

QCoro::Task<void> military_unit::check_promotions()
{
	co_await this->check_free_promotions();

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
			co_await this->remove_promotion(promotion);
		}

		//check promotions again, as the removal of a promotion might have invalidated other ones
		co_await this->check_promotions();
	}
}

QCoro::Task<void> military_unit::check_free_promotions()
{
	bool changed = false;

	for (const promotion *promotion : this->get_type()->get_free_promotions()) {
		if (this->has_promotion(promotion)) {
			continue;
		}

		if (!this->can_have_promotion(promotion)) {
			continue;
		}

		co_await this->add_promotion(promotion);
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

			co_await this->add_promotion(promotion);
			changed = true;
		}
	}

	if (changed) {
		//check free promotions again, as the addition of a free promotion might have caused the requirements of others to be fulfilled
		co_await this->check_free_promotions();
	}
}

QCoro::Task<void> military_unit::attack(military_unit *target, const bool ranged, const bool moved, const int to_hit_modifier) const
{
	assert_throw(target != nullptr);

	if (this->get_character() != nullptr && target->get_character() != nullptr) {
		//attack between characters
		co_await this->attack_character(target->get_character(), to_hit_modifier);
		co_return;
	}

	int attack = 0;
	if (ranged) {
		attack = this->get_effective_stat(military_unit_stat::missile).to_int();
	} else if (moved && this->get_effective_stat(military_unit_stat::charge).to_int() > 0) {
		attack = this->get_effective_stat(military_unit_stat::charge).to_int();
	} else {
		attack = this->get_effective_stat(military_unit_stat::melee).to_int();
		if (target->get_type()->is_cavalry()) {
			attack += this->get_effective_stat(military_unit_stat::melee_vs_mounted).to_int();
		}
	}

	int defense = target->get_effective_stat(military_unit_stat::defense).to_int();
	if (this->get_type()->is_cavalry()) {
		defense += target->get_effective_stat(military_unit_stat::defense_vs_mounted).to_int();
	}

	const std::unique_ptr<battle_resolution_table> &battle_resolution_table = vector::get_random(defines::get()->get_battle_resolution_tables());

	const attack_result result = battle_resolution_table->get_result(this->get_battle_resolution_type(), target->get_battle_resolution_type(), attack - defense);

	switch (result) {
		case attack_result::miss:
		case attack_result::fall_back:
			break;
		case attack_result::hit:
		case attack_result::rout:
			co_await target->receive_damage(1);
			break;
		case attack_result::destroy:
			co_await target->die();
			break;
	}
}

QCoro::Task<void> military_unit::attack_character(const metternich::character *target_character, const int to_hit_modifier) const
{
	assert_throw(this->get_character() != nullptr);

	const bool hit = this->check_to_hit(target_character, to_hit_modifier);

	if (!hit) {
		co_return;
	}

	//perform attack between characters
	const int damage = random::get()->roll_dice(this->get_character()->get_game_data()->get_damage_dice()) + this->get_character()->get_game_data()->get_damage_bonus();
	co_await target_character->get_game_data()->change_health(-damage);
}

bool military_unit::check_to_hit(const metternich::character *target_character, const int to_hit_modifier) const
{
	assert_throw(this->get_character() != nullptr);

	static constexpr dice to_hit_dice(1, 20);
	const int to_hit = 20 - this->get_character()->get_game_data()->get_to_hit_bonus() - to_hit_modifier;
	const int to_hit_result = to_hit - random::get()->roll_dice(to_hit_dice);

	const int armor_class_bonus = target_character->get_game_data()->get_armor_class_bonus() + target_character->get_game_data()->get_species_armor_class_bonus(this->get_character()->get_species());
	const int armor_class = 10 - armor_class_bonus;
	if (to_hit_result > armor_class) {
		return false;
	}

	return true;
}

QCoro::Task<void> military_unit::receive_damage(const int damage)
{
	if (this->get_character() != nullptr) {
		const int health_damage = damage * character_defines::get()->get_battle_hit_point_rate();
		co_await this->get_character()->get_game_data()->change_health(-health_damage);
	} else {
		co_await this->change_hit_points(-damage);
	}
}

QCoro::Task<void> military_unit::heal(const int healing)
{
	const int missing_hit_points = this->get_max_hit_points() - this->get_hit_points();

	if (missing_hit_points == 0) {
		co_return;
	}

	co_await this->change_hit_points(std::min(healing, missing_hit_points));
}

QCoro::Task<void> military_unit::die()
{
	co_await this->change_hit_points(-this->get_hit_points());
}

QCoro::Task<void> military_unit::disband(const bool dead)
{
	if (this->get_character() != nullptr) {
		character_game_data *character_game_data = this->get_character()->get_game_data();
		co_await character_game_data->set_military_unit(nullptr);

		if (dead) {
			co_await character_game_data->die();
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
		co_await this->get_country()->get_military()->remove_military_unit(this);
	}
}

int military_unit::get_score() const
{
	int score = 0;

	for (const auto &[stat, stat_value] : this->stats) {
		if (is_percent_military_unit_stat(stat)) {
			score += stat_value.to_int() / 10;
		} else {
			score += (stat_value * 25).to_int();
		}
	}

	return score;
}

std::string military_unit::get_stats_string(const bool in_battle) const
{
	std::string str;

	str += std::format("{}/{} {}", this->get_hit_points(), this->get_max_hit_points(), get_military_unit_stat_short_name(military_unit_stat::hit_points));

	for (const auto &[stat, stat_value] : this->stats) {
		if (stat == military_unit_stat::hit_points) {
			//already written
			continue;
		}

		if (!str.empty()) {
			str += ", ";
		}

		int stat_value_int = 0;
		if (stat == military_unit_stat::movement && in_battle) {
			stat_value_int = this->get_battle_movement();
		} else {
			stat_value_int = stat_value.to_int();
		}

		str += std::format("{} {}", stat_value_int, get_military_unit_stat_short_name(stat));
	}

	return str;
}

QString military_unit::get_stats_qstring() const
{
	return QString::fromStdString(this->get_stats_string(false));
}

const sound *military_unit::get_melee_attack_sound() const
{
	if (this->get_character() != nullptr && this->get_character()->get_game_data()->get_attack_sound() != nullptr) {
		return this->get_character()->get_game_data()->get_attack_sound();
	}

	return this->get_type()->get_melee_attack_sound();
}

const sound *military_unit::get_ranged_attack_sound() const
{
	if (this->get_character() != nullptr && this->get_character()->get_game_data()->get_attack_sound() != nullptr) {
		return this->get_character()->get_game_data()->get_attack_sound();
	}

	return this->get_type()->get_ranged_attack_sound();
}

const sound *military_unit::get_death_sound() const
{
	return this->get_type()->get_death_sound();
}

}
