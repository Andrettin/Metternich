#pragma once

#include "database/defines_base.h"
#include "util/dice.h"
#include "util/singleton.h"

namespace metternich {

class character;
enum class bloodline_strength_category;
enum class divine_rank;

template <typename scope_type>
class modifier;

class character_defines final : public defines_base, public singleton<character_defines>
{
	Q_OBJECT

	Q_PROPERTY(int craft_recovery_per_day MEMBER craft_recovery_per_day READ get_craft_recovery_per_day NOTIFY changed)
	Q_PROPERTY(int battle_hit_point_rate MEMBER battle_hit_point_rate READ get_battle_hit_point_rate NOTIFY changed)
	Q_PROPERTY(archimedes::dice ruler_reputation_dice MEMBER ruler_reputation_dice READ get_ruler_reputation_dice NOTIFY changed)
	Q_PROPERTY(int max_character_normal_level MEMBER max_character_normal_level READ get_max_character_normal_level NOTIFY changed)

public:
	character_defines();
	~character_defines();

	virtual std::string_view get_file_name() const override
	{
		return "character_defines.txt";
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;

	int get_minimum_character_range() const
	{
		return this->minimum_character_range;
	}

	int get_craft_recovery_per_day() const
	{
		return this->craft_recovery_per_day;
	}

	int get_battle_hit_point_rate() const
	{
		return this->battle_hit_point_rate;
	}

	int get_battle_movement_rate() const
	{
		return this->battle_movement_rate;
	}

	const dice &get_ruler_reputation_dice() const
	{
		return this->ruler_reputation_dice;
	}

	int get_max_character_normal_level() const
	{
		return this->max_character_normal_level;
	}

	int64_t get_experience_for_level(const int level) const
	{
		const auto find_iterator = this->experience_per_level.find(level);
		if (find_iterator != this->experience_per_level.end()) {
			return find_iterator->second;
		}

		if (level <= 0) {
			throw std::runtime_error(std::format("No experience total is given for level {}.", level));
		}

		const int64_t previous_level_experience = this->get_experience_for_level(level - 1);
		return (previous_level_experience - this->get_experience_for_level(level - 2)) * 2 + previous_level_experience;
	}

	int64_t get_experience_award_for_challenge_rating(const int challenge_rating) const
	{
		const auto find_iterator = this->experience_award_per_challenge_rating.find(challenge_rating);
		if (find_iterator != this->experience_award_per_challenge_rating.end()) {
			return find_iterator->second;
		}

		if (challenge_rating <= 0) {
			throw std::runtime_error(std::format("No experience award is given for challenge rating {}.", challenge_rating));
		}

		const int64_t previous_experience_award = this->get_experience_award_for_challenge_rating(challenge_rating - 1);
		return previous_experience_award + 1000;
	}

	int get_bloodline_strength_category_weight(const bloodline_strength_category category) const;
	const std::vector<bloodline_strength_category> &get_weighted_bloodline_strength_categories() const;
	const dice &get_bloodline_strength_for_category(const bloodline_strength_category category) const;

	int get_divine_rank_level(const divine_rank rank) const;
	divine_rank get_divine_level_rank(const int divine_level) const;
	const metternich::modifier<const character> *get_divine_rank_modifier(const int divine_rank) const;

	int get_mana_cost_for_spell_level(const int level) const;

	int get_battle_attack_conversion_points_for_to_hit_bonus(int to_hit_bonus) const;
	int get_battle_attack_conversion_points_for_max_damage(int max_damage) const;
	int get_battle_melee_for_to_hit_bonus_and_max_damage(const int to_hit_bonus, const int max_damage) const;
	int get_battle_missile_for_to_hit_bonus_and_max_damage(const int to_hit_bonus, const int max_damage) const;
	int get_battle_defense_for_armor_class(const int armor_class) const;

signals:
	void changed();

private:
	int minimum_character_range = 0;
	int craft_recovery_per_day = 0;
	int battle_hit_point_rate = 0; //character health per military unit hit point
	int battle_movement_rate = 0; //movement in battle per character movement point, in feet
	dice ruler_reputation_dice;
	int max_character_normal_level = 0;
	std::map<int, int64_t> experience_per_level;
	std::map<int, int64_t> experience_award_per_challenge_rating;
	std::map<bloodline_strength_category, int> bloodline_strength_category_weights;
	std::vector<bloodline_strength_category> weighted_bloodline_strength_categories;
	std::map<bloodline_strength_category, dice> bloodline_strength_per_category;
	std::map<divine_rank, int> divine_rank_levels;
	std::map<int, std::unique_ptr<const modifier<const character>>> divine_rank_modifiers;
	std::map<int, int> mana_cost_per_spell_level;
	std::map<int, int> battle_attack_conversion_points_per_to_hit_bonus;
	std::map<int, int> battle_attack_conversion_points_per_max_damage;
	std::map<int, int> battle_melee_per_attack_conversion_points;
	std::map<int, int> battle_missile_per_attack_conversion_points;
	std::map<int, int> battle_defense_per_armor_class; //military unit defense per character armor class
};

}
