#pragma once

#include "character/character.h"
#include "population/population_unit.h"
#include "script/condition/condition.h"
#include "species/species.h"

namespace metternich {

template <typename scope_type>
class species_condition final : public condition<scope_type>
{
public:
	explicit species_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->species = species::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "species";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_species() == this->species;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} species", this->species->get_name());
	}

private:
	const metternich::species *species = nullptr;
};

}
