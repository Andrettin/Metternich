#pragma once

#include "infrastructure/pathway.h"
#include "script/condition/condition.h"

namespace metternich {

class has_pathway_condition final : public condition<site>
{
public:
	explicit has_pathway_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->pathway = pathway::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_pathway";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->has_pathway(this->pathway);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} pathway", this->pathway->get_name());
	}

private:
	const metternich::pathway *pathway = nullptr;
};

}
