#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "script/effect/effect.h"
#include "util/dice.h"
#include "util/random.h"

namespace metternich {

class damage_effect final : public effect<const character>
{
public:
	using damage_variant = std::variant<int, dice>;

	explicit damage_effect(const gsml_operator effect_operator) : effect(effect_operator)
	{
	}

	explicit damage_effect(const std::string &value, const gsml_operator effect_operator)
		: damage_effect(effect_operator)
	{
		this->damage = damage_effect::string_to_damage_variant(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "damage";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "value") {
			this->damage = damage_effect::string_to_damage_variant(value);
		} else if (key == "value_per_caster_level") {
			this->damage_per_caster_level = std::stoi(value);
		} else {
			effect::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "caster_level_values") {
			scope.for_each_property([this](const gsml_property &property) {
				const int level = std::stoi(property.get_key());
				this->caster_level_damage[level] = damage_effect::string_to_damage_variant(property.get_value());
			});
		} else {
			effect::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(const character *scope, context &ctx) const override
	{
		int damage = damage_effect::damage_variant_to_damage(this->damage);

		if (std::holds_alternative<const character *>(ctx.source_scope)) {
			const character *caster = std::get<const character *>(ctx.source_scope);
			const int caster_level = caster->get_game_data()->get_level();
			damage += caster_level * this->damage_per_caster_level;

			for (const auto &[loop_caster_level, caster_level_damage] : this->caster_level_damage) {
				if (loop_caster_level > caster_level) {
					break;
				}

				damage += damage_effect::damage_variant_to_damage(caster_level_damage);
			}
		}

		scope->get_game_data()->change_health(-damage);
	}

	virtual std::string get_assignment_string(const character *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(prefix);
		Q_UNUSED(indent);

		int min_damage = 0;
		int max_damage = 0;

		damage_effect::add_damage_variant_to_min_max_damage(this->damage, min_damage, max_damage);

		if (std::holds_alternative<const character *>(ctx.source_scope)) {
			const character *caster = std::get<const character *>(ctx.source_scope);
			const int caster_level = caster->get_game_data()->get_level();
			const int damage_bonus = caster_level * this->damage_per_caster_level;
			min_damage += damage_bonus;
			max_damage += damage_bonus;

			for (const auto &[loop_caster_level, caster_level_damage] : this->caster_level_damage) {
				if (loop_caster_level > caster_level) {
					break;
				}

				damage_effect::add_damage_variant_to_min_max_damage(caster_level_damage, min_damage, max_damage);
			}
		}

		std::string quantity_string;
		if (min_damage == max_damage) {
			quantity_string = std::to_string(min_damage);
		} else {
			quantity_string = std::format("{}-{}", min_damage, max_damage);
		}

		return std::format("Receive {} Damage", quantity_string);
	}

	static damage_variant string_to_damage_variant(const std::string &str)
	{
		if (string::is_number(str)) {
			return std::stoi(str);
		} else {
			return dice(str);
		}
	}

	static int damage_variant_to_damage(const damage_variant &damage_variant)
	{
		if (std::holds_alternative<int>(damage_variant)) {
			return std::get<int>(damage_variant);
		} else {
			const dice &damage_dice = std::get<dice>(damage_variant);
			return random::get()->roll_dice(damage_dice);
		}
	}

	static void add_damage_variant_to_min_max_damage(const damage_variant &damage_variant, int &min_damage, int &max_damage)
	{
		if (std::holds_alternative<int>(damage_variant)) {
			const int damage = std::get<int>(damage_variant);
			min_damage += damage;
			max_damage += damage;
		} else {
			const dice &damage_dice = std::get<dice>(damage_variant);
			min_damage += damage_dice.get_minimum_result();
			max_damage += damage_dice.get_maximum_result();
		}
	}

private:
	damage_variant damage;
	int damage_per_caster_level = 0;
	std::map<int, damage_variant> caster_level_damage;
};

}
