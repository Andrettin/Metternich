#pragma once

#include "script/effect/effect.h"
#include "script/special_target_type.h"
#include "script/target_variant.h"
#include "util/assert_util.h"
#include "util/number_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

template <typename scope_type>
class change_opinion_effect final : public effect<scope_type>
{
public:
	explicit change_opinion_effect(const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "change_opinion";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "target") {
			if (enum_converter<special_target_type>::has_value(value)) {
				this->target = enum_converter<special_target_type>::to_enum(value);
			} else {
				this->target = std::remove_const_t<scope_type>::get(value);
			}
		} else if (key == "value") {
			this->value = std::stoi(value);
		} else if (key == "mutual") {
			this->mutual = string::to_bool(value);
		} else {
			assert_throw(false);
		}
	}

	virtual void check() const override
	{
		if (std::holds_alternative<std::monostate>(this->target)) {
			throw std::runtime_error("Change opinion effect has no target.");
		}
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

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		const scope_type *target_scope = this->get_target_scope(ctx);

		if (target_scope == nullptr) {
			return;
		}

		scope->get_game_data()->change_base_opinion(target_scope, this->value);

		if (this->mutual) {
			target_scope->get_game_data()->change_base_opinion(scope, this->value);
		}
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		if (this->mutual) {
			return std::format("{} mutual {} with {}", number::to_signed_string(this->value), string::highlight("Opinion"), string::highlight(this->get_target_name(ctx)));
		} else {
			return std::format("{} {} towards {}", number::to_signed_string(this->value), string::highlight("Opinion"), string::highlight(this->get_target_name(ctx)));
		}
	}

private:
	target_variant<scope_type> target;
	int value = 0;
	bool mutual = false;
};

}
