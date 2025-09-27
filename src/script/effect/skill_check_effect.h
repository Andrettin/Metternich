#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/skill.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "script/context.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "util/assert_util.h"
#include "util/random.h"

namespace metternich {

template <typename scope_type> 
class skill_check_effect final : public effect<scope_type>
{
public:
	explicit skill_check_effect(const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "skill_check";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "skill") {
			this->skill = skill::get(value);
		} else if (key == "roll_modifier") {
			this->roll_modifier = std::stoi(value);
		} else {
			effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "on_success") {
			this->success_effects = std::make_unique<effect_list<scope_type>>();
			this->success_effects->process_gsml_data(scope);
		} else if (tag == "on_failure") {
			this->failure_effects = std::make_unique<effect_list<scope_type>>();
			this->failure_effects->process_gsml_data(scope);
		} else {
			effect<scope_type>::process_gsml_scope(scope);
		}
	}

	virtual void check() const override
	{
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		const character *roll_character = this->get_roll_character(scope, ctx);
		const bool success = roll_character->get_game_data()->do_skill_check(this->skill, this->roll_modifier);

		if (success) {
			if (this->success_effects != nullptr) {
				this->success_effects->do_effects(scope, ctx);
			}
		} else {
			if (this->failure_effects != nullptr) {
				this->failure_effects->do_effects(scope, ctx);
			}
		}
	}

	virtual std::string get_assignment_string(scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		const character *roll_character = this->get_roll_character(scope, ctx);

		std::string str = std::format("{} ({}{}) Check", this->skill->get_name(), roll_character->get_game_data()->is_skill_trained(this->skill) ? roll_character->get_game_data()->get_skill_value(this->skill) : 0, this->skill->get_value_suffix());
		str += "\n" + std::string(indent + 1, '\t');

		if (this->roll_modifier != 0) {
			str += std::format("Roll Modifier: {}{}", number::to_signed_string(this->roll_modifier), this->skill->get_value_suffix());
		}

		if (this->success_effects != nullptr) {
			const std::string effects_string = this->success_effects->get_effects_string(scope, ctx, indent + 1, prefix);
			if (!effects_string.empty()) {
				str += "\n" + std::string(indent, '\t') + "If successful:\n" + effects_string;
			}
		}

		if (this->failure_effects != nullptr) {
			const std::string effects_string = this->failure_effects->get_effects_string(scope, ctx, indent + 1, prefix);
			if (!effects_string.empty()) {
				str += "\n" + std::string(indent, '\t') + "If failed:\n" + effects_string;
			}
		}

		return str;
	}

	const character *get_roll_character(scope_type *scope, const read_only_context &ctx) const
	{
		const character *roll_character = nullptr;

		if constexpr (std::is_same_v<scope_type, const character>) {
			roll_character = scope;
		} else {
			assert_throw(ctx.party != nullptr);
			roll_character = ctx.party->get_best_skill_character(this->skill);
		}

		assert_throw(roll_character != nullptr);

		return roll_character;
	}

private:
	const metternich::skill *skill = nullptr;
	int roll_modifier = 0;
	std::unique_ptr<effect_list<scope_type>> success_effects;
	std::unique_ptr<effect_list<scope_type>> failure_effects;
};

}
