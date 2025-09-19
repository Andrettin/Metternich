#pragma once

#include "domain/country_government.h"
#include "domain/domain.h"
#include "script/effect/scope_effect.h"

namespace metternich {

class character;
class domain;
class office;

class office_holder_effect final : public scope_effect<const domain, const character>
{
public:
	explicit office_holder_effect(const metternich::office *office, const gsml_operator effect_operator)
		: scope_effect<const domain, const character>(effect_operator), office(office)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "office_holder";
		return class_identifier;
	}
	
	virtual const character *get_scope(const domain *upper_scope) const override
	{
		return upper_scope->get_government()->get_office_holder(this->office);
	}

private:
	const metternich::office *office = nullptr;
};

}
