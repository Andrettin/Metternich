#pragma once

#include "map/site.h"
#include "map/site_attribute.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

template <typename scope_type>
class site_attribute_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit site_attribute_modifier_effect(const site_attribute *attribute, const std::string &value)
		: modifier_effect<scope_type>(value), attribute(attribute)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "site_attribute";
		return identifier;
	}

	virtual void apply(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		if constexpr (std::is_same_v<scope_type, const site>) {
			scope->get_game_data()->change_attribute_value(this->attribute, (this->value * multiplier).to_int());
		} else {
			scope->get_game_data()->change_site_attribute_value(this->attribute, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return this->attribute->get_name();
	}

private:
	const site_attribute *attribute = nullptr;
};

}
