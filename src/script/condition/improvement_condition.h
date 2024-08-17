#pragma once

#include "infrastructure/improvement.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

class improvement_condition final : public condition<site>
{
public:
	explicit improvement_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->improvement = improvement::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "improvement";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_improvement(this->improvement->get_slot()) == this->improvement;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->improvement->get_name() + " improvement";
	}

private:
	const metternich::improvement *improvement = nullptr;
};

}
