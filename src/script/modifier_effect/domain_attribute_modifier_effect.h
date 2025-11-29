#pragma once

#include "domain/domain.h"
#include "domain/domain_attribute.h"
#include "domain/domain_game_data.h"
#include "script/modifier_effect/modifier_effect.h"

namespace metternich {

class domain_attribute_modifier_effect final : public modifier_effect<const domain>
{
public:
	explicit domain_attribute_modifier_effect(const domain_attribute *attribute, const std::string &value)
		: modifier_effect<const domain>(value), attribute(attribute)
	{
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "domain_attribute";
		return identifier;
	}

	virtual void apply(const domain *scope, const centesimal_int &multiplier) const override
	{
		scope->get_game_data()->change_attribute_value(this->attribute, (this->value * multiplier).to_int());
	}

	virtual std::string get_base_string(const domain *scope) const override
	{
		Q_UNUSED(scope);

		return this->attribute->get_name();
	}

private:
	const domain_attribute *attribute = nullptr;
};

}
