#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/defines.h"
#include "script/effect/effect.h"
#include "script/opinion_modifier.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class opinion_modifiers_effect final : public effect<const character>
{
public:
	enum class target_type {
		none,
		root
	};

	explicit opinion_modifiers_effect(const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "opinion_modifiers";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "modifier") {
			this->modifier = opinion_modifier::get(value);
		} else if (key == "target") {
			if (value == "root") {
				this->target = target_type::root;
			} else {
				assert_throw(false);
			}
		} else if (key == "duration") {
			this->duration = std::stoi(value);
		} else if (key == "days") {
			const int value_int = std::stoi(value);
			this->duration = defines::get()->days_to_turns(value_int).to_int();

			if (value_int > 0) {
				this->duration = std::max(1, duration);
			}
		} else if (key == "months") {
			const int value_int = std::stoi(value);
			this->duration = defines::get()->months_to_turns(value_int).to_int();

			if (value_int > 0) {
				this->duration = std::max(1, duration);
			}
		} else if (key == "years") {
			const int value_int = std::stoi(value);
			this->duration = defines::get()->years_to_turns(value_int).to_int();

			if (value_int > 0) {
				this->duration = std::max(1, duration);
			}
		}
	}

	virtual void check() const override
	{
		if (this->target == target_type::none) {
			throw std::runtime_error("Opinion modifier effect has no target.");
		}

		if (this->get_operator() == gsml_operator::addition && this->duration == 0) {
			throw std::runtime_error("Add opinion modifier effect has no duration.");
		}
	}

	const scope_type *get_target_scope(const read_only_context &ctx) const
	{
		switch (this->target) {
			case target_type::root:
				if constexpr (std::is_same_v<scope_type, const character>) {
					return ctx.current_character;
				} else if constexpr (std::is_same_v<scope_type, const country>) {
					return ctx.current_country;
				}
				break;
			default:
				break;
		}

		assert_throw(false);
		return nullptr;
	}

	std::string get_target_name(const read_only_context &ctx) const
	{
		const scope_type *target_scope = this->get_target_scope(ctx);

		if constexpr (std::is_same_v<scope_type, const character>) {
			return target_scope->get_full_name();
		} else {
			return target_scope->get_name();
		}
	}

	virtual void do_addition_effect(const scope_type *scope, context &ctx) const override
	{
		scope->get_game_data()->add_opinion_modifier(this->get_target_scope(ctx), this->modifier, this->duration ? this->duration : this->modifier->get_duration());
	}

	virtual void do_subtraction_effect(const scope_type *scope, context &ctx) const override
	{
		scope->get_game_data()->remove_opinion_modifier(this->get_target_scope(ctx), this->modifier);
	}

	virtual std::string get_addition_string(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);

		return "Gain the " + this->modifier->get_name() + " opinion modifier towards " + this->get_target_name(ctx) + " for " + std::to_string(this->duration * defines::get()->get_months_per_turn()) + " months";
	}

	virtual std::string get_subtraction_string(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);

		return "Lose the " + this->modifier->get_name() + " opinion modifier towards " + this->get_target_name(ctx);
	}

private:
	const opinion_modifier *modifier = nullptr;
	target_type target = target_type::none;
	int duration = 0;
};

}
