#pragma once

#include "map/province.h"
#include "map/province_attribute.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class province_attribute_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit province_attribute_modifier_effect(const province_attribute *attribute, const std::string &value)
		: modifier_effect<scope_type>(value), attribute(attribute)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "province_attribute";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		if constexpr (std::is_same_v<scope_type, const province>) {
			scope->get_game_data()->change_attribute_value(this->attribute, (this->value * multiplier).to_int());
		} else {
			scope->get_game_data()->change_province_attribute_value(this->attribute, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return this->attribute->get_name();
	}

private:
	const province_attribute *attribute = nullptr;
};

}
