#pragma once

#include "map/province.h"
#include "map/province_attribute.h"
#include "map/province_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class province_attribute_modifier_effect final : public modifier_effect<const province>
{
public:
	explicit province_attribute_modifier_effect(const province_attribute *attribute, const std::string &value)
		: modifier_effect<const province>(value), attribute(attribute)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "province_attribute";
		return identifier;
	}

	virtual void apply(const province *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_attribute_value(this->attribute, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const province *scope) const override
	{
		Q_UNUSED(scope);

		return this->attribute->get_name();
	}

private:
	const province_attribute *attribute = nullptr;
};

}
