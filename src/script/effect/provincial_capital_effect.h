#pragma once

#include "map/site.h"
#include "script/effect/effect.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class provincial_capital_effect final : public effect<scope_type>
{
public:
	using value_type = std::conditional_t<std::is_same_v<scope_type, const province>, const site *, bool>;

	explicit provincial_capital_effect(const value_type value, const gsml_operator effect_operator = gsml_operator::assignment)
		: effect<scope_type>(effect_operator), value(value)
	{
	}

	explicit provincial_capital_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
		if constexpr (std::is_same_v<scope_type, const province>) {
			this->value = site::get(value);
		} else {
			this->value = string::to_bool(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "provincial_capital";
		return class_identifier;
	}

	virtual void do_assignment_effect(const scope_type *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		if constexpr (std::is_same_v<scope_type, const province>) {
			if (this->value->get_game_data()->get_province() != scope) {
				return;
			}

			if (scope->get_game_data()->is_capital()) {
				//cannot change the provincial capital if that would cause it to be different than the country's capital, if the scope is the capital province
				return;
			}

			scope->get_game_data()->set_provincial_capital(this->value);
		} else {
			const province *province = scope->get_game_data()->get_province();
			if (province != nullptr && !province->get_game_data()->is_capital()) {
				province->get_game_data()->set_provincial_capital(scope);
			}
		}
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		std::string provincial_capital_name;

		if constexpr (std::is_same_v<scope_type, const province>) {
			provincial_capital_name = this->value->get_name();
		} else {
			provincial_capital_name = scope->get_name();
		}

		return std::format("{} becomes the provincial capital", string::highlight(provincial_capital_name));
	}


private:
	value_type value{};
};

}
