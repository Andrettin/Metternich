#pragma once

#include "character/character.h"
#include "population/population_unit.h"
#include "script/condition/condition.h"
#include "species/taxon.h"

namespace metternich {

template <typename scope_type>
class taxon_condition final : public condition<scope_type>
{
public:
	explicit taxon_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->taxon = taxon::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "taxon";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_species()->is_subtaxon_of(this->taxon);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} taxon", this->taxon->get_name());
	}

private:
	const metternich::taxon *taxon = nullptr;
};

}
