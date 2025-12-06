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
		} else if (tag == "on_success_character") {
			this->character_success_effects = std::make_unique<effect_list<const character>>();
			this->character_success_effects->process_gsml_data(scope);
		} else if (tag == "on_failure") {
			this->failure_effects = std::make_unique<effect_list<scope_type>>();
			this->failure_effects->process_gsml_data(scope);
		} else if (tag == "on_failure_character") {
			this->character_failure_effects = std::make_unique<effect_list<const character>>();
			this->character_failure_effects->process_gsml_data(scope);
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
		const bool success = roll_character->get_game_data()->do_skill_check(this->skill, this->roll_modifier, this->get_location(roll_character, ctx));

		bool is_player = false;
		if constexpr (std::is_same_v<scope_type, const character>) {
			is_player = scope == game::get()->get_player_character();
		} else if constexpr (std::is_same_v<scope_type, const domain>) {
			is_player = scope == game::get()->get_player_country();
		}

		if (is_player) {
			const domain *domain = effect<scope_type>::get_scope_domain(scope);
			const portrait *interior_minister_portrait = domain->get_government()->get_interior_minister_portrait();

			const std::string effects_string = success ? this->get_success_effects_string(scope, ctx, 0, "") : this->get_failure_effects_string(scope, ctx, 0, "");

			if (success) {
				engine_interface::get()->add_notification("Skill Check Successful!", interior_minister_portrait, std::format("You have succeeded in a {} skill check!{}", this->skill->get_name(), !effects_string.empty() ? ("\n\n" + effects_string) : ""));
			} else {
				engine_interface::get()->add_notification("Skill Check Failed!", interior_minister_portrait, std::format("You have failed a {} skill check!{}", this->skill->get_name(), !effects_string.empty() ? ("\n\n" + effects_string) : ""));
			}
		}

		if (success) {
			if (this->success_effects != nullptr) {
				this->success_effects->do_effects(scope, ctx);
			}

			if (this->character_success_effects != nullptr) {
				this->character_success_effects->do_effects(roll_character, ctx);
			}
		} else {
			if (this->failure_effects != nullptr) {
				this->failure_effects->do_effects(scope, ctx);
			}

			if (this->character_failure_effects != nullptr) {
				this->character_failure_effects->do_effects(roll_character, ctx);
			}
		}
	}

	virtual std::string get_assignment_string(scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		const character *roll_character = this->get_roll_character(scope, ctx);

		std::string str = std::format("{} Check ({}% Chance)", this->skill->get_name(), roll_character->get_game_data()->get_skill_check_chance(this->skill, this->roll_modifier, this->get_location(roll_character, ctx)));

		if (this->roll_modifier != 0) {
			str += "\n" + std::string(indent + 1, '\t') + std::format("Roll Modifier: {}{}", number::to_signed_string(this->roll_modifier), this->skill->get_value_suffix());
		}

		const std::string success_effects_string = this->get_success_effects_string(scope, ctx, indent + 1, prefix);
		if (!success_effects_string.empty()) {
			str += "\n" + std::string(indent, '\t') + "If successful:\n" + success_effects_string;
		}

		const std::string failure_effects_string = this->get_failure_effects_string(scope, ctx, indent + 1, prefix);
		if (!failure_effects_string.empty()) {
			str += "\n" + std::string(indent, '\t') + "If failed:\n" + failure_effects_string;
		}

		return str;
	}

	std::string get_success_effects_string(scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
	{
		std::string success_effects_string;

		if (this->success_effects != nullptr) {
			success_effects_string = this->success_effects->get_effects_string(scope, ctx, indent, prefix);
		}

		if (this->character_success_effects != nullptr) {
			if (!success_effects_string.empty()) {
				success_effects_string += "\n";
			}

			const character *roll_character = this->get_roll_character(scope, ctx);
			success_effects_string += std::format("{}{}:\n", std::string(indent, '\t'), roll_character->get_full_name()) + this->character_success_effects->get_effects_string(roll_character, ctx, indent + 1, prefix);
		}

		return success_effects_string;
	}

	std::string get_failure_effects_string(scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const
	{
		std::string failure_effects_string;

		if (this->failure_effects != nullptr) {
			failure_effects_string = this->failure_effects->get_effects_string(scope, ctx, indent, prefix);
		}

		if (this->character_failure_effects != nullptr) {
			if (!failure_effects_string.empty()) {
				failure_effects_string += "\n";
			}

			const character *roll_character = this->get_roll_character(scope, ctx);
			failure_effects_string += std::format("{}{}:\n", std::string(indent, '\t'), roll_character->get_full_name()) + this->character_failure_effects->get_effects_string(roll_character, ctx, indent + 1, prefix);
		}

		return failure_effects_string;
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

	const province *get_location(const character *roll_character, const read_only_context &ctx) const
	{
		assert_throw(ctx.dungeon_site != nullptr || roll_character->get_game_data()->get_domain() != nullptr);
		if (ctx.dungeon_site != nullptr) {
			return ctx.dungeon_site->get_game_data()->get_province();
		}

		return roll_character->get_game_data()->get_location();
	}

private:
	const metternich::skill *skill = nullptr;
	int roll_modifier = 0;
	std::unique_ptr<effect_list<scope_type>> success_effects;
	std::unique_ptr<effect_list<const character>> character_success_effects;
	std::unique_ptr<effect_list<scope_type>> failure_effects;
	std::unique_ptr<effect_list<const character>> character_failure_effects;
};

}
