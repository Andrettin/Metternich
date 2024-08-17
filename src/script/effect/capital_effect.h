#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/effect/effect.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class capital_effect final : public effect<scope_type>
{
public:
	using value_type = std::conditional_t<std::is_same_v<scope_type, const country>, const site *, bool>;

	explicit capital_effect(const value_type value, const gsml_operator effect_operator = gsml_operator::assignment)
		: effect<scope_type>(effect_operator), value(value)
	{
	}

	explicit capital_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
		if constexpr (std::is_same_v<scope_type, const country>) {
			this->value = site::get(value);
		} else {
			this->value = string::to_bool(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "capital";
		return class_identifier;
	}

	virtual void do_assignment_effect(const scope_type *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		const site *settlement = nullptr;
		const country *country = nullptr;

		if constexpr (std::is_same_v<scope_type, const metternich::country>) {
			settlement = this->value;
			country = scope;
		} else {
			settlement = scope;
			country = scope->get_game_data()->get_owner();
		}

		if (country == nullptr) {
			return;
		}

		if (settlement->get_game_data()->get_owner() != country) {
			return;
		}

		if (!settlement->get_game_data()->can_be_capital()) {
			return;
		}

		country->get_game_data()->set_capital(settlement);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		std::string capital_name;

		if constexpr (std::is_same_v<scope_type, const country>) {
			capital_name = this->value->get_name();
		} else {
			capital_name = scope->get_name();
		}

		return std::format("{} becomes the capital", string::highlight(capital_name));
	}


private:
	value_type value{};
};

}
