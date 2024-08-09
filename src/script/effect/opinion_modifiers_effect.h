#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "database/defines.h"
#include "script/effect/effect.h"
#include "script/opinion_modifier.h"
#include "script/special_target_type.h"
#include "script/target_variant.h"
#include "util/assert_util.h"
#include "util/number_util.h"

namespace metternich {

template <typename scope_type>
class opinion_modifiers_effect final : public effect<scope_type>
{
public:
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
			if (enum_converter<special_target_type>::has_value(value)) {
				this->target = enum_converter<special_target_type>::to_enum(value);
			} else {
				this->target = std::remove_const_t<scope_type>::get(value);
			}
		} else if (key == "duration") {
			this->duration = std::stoi(value);
		} else if (key == "days") {
			this->months_duration = centesimal_int(value) / 30;
		} else if (key == "months") {
			this->months_duration = centesimal_int(value);
		} else if (key == "years") {
			this->months_duration = centesimal_int(value) * 12;
		}
	}

	virtual void check() const override
	{
		if (std::holds_alternative<std::monostate>(this->target)) {
			throw std::runtime_error("Opinion modifier effect has no target.");
		}

		if (this->get_operator() == gsml_operator::addition && this->duration == 0 && this->months_duration == 0) {
			throw std::runtime_error("Add opinion modifier effect has no duration.");
		}
	}

	int get_duration() const
	{
		if (this->duration > 0) {
			return this->duration;
		}

		if (this->months_duration > 0) {
			return centesimal_int::max(defines::get()->months_to_turns(this->months_duration, game::get()->get_year()), 1).to_int();
		}

		if (this->modifier->get_duration() > 0) {
			return this->modifier->get_duration();
		}

		if (this->modifier->get_duration_days() > 0) {
			return centesimal_int::max(defines::get()->days_to_turns(this->modifier->get_duration_days(), game::get()->get_year()), 1).to_int();
		}

		return 0;
	}

	scope_type *get_target_scope(const context &ctx) const
	{
		return effect<scope_type>::get_target_scope(this->target, ctx);
	}

	std::string get_target_name(const read_only_context &ctx) const
	{
		const scope_type *target_scope = effect<scope_type>::get_target_scope(this->target, ctx);
		return target_scope->get_scope_name();
	}

	virtual void do_addition_effect(const scope_type *scope, context &ctx) const override
	{
		scope->get_game_data()->add_opinion_modifier(this->get_target_scope(ctx), this->modifier, this->get_duration());
	}

	virtual void do_subtraction_effect(const scope_type *scope, context &ctx) const override
	{
		scope->get_game_data()->remove_opinion_modifier(this->get_target_scope(ctx), this->modifier);
	}

	virtual std::string get_addition_string(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);

		return std::format("{} Opinion towards {} for {} turns", number::to_signed_string(this->modifier->get_value()), this->get_target_name(ctx), this->get_duration());
	}

	virtual std::string get_subtraction_string(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);

		return "Lose the " + this->modifier->get_name() + " opinion modifier towards " + this->get_target_name(ctx);
	}

private:
	const opinion_modifier *modifier = nullptr;
	target_variant<scope_type> target;
	int duration = 0;
	centesimal_int months_duration;
};

}
