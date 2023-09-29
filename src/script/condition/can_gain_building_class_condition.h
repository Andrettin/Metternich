#pragma once

#include "infrastructure/building_class.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

class can_gain_building_class_condition final : public condition<site>
{
public:
	explicit can_gain_building_class_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->building_class = building_class::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "can_gain_building_class";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->can_gain_building_class(this->building_class);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("Can gain the {} building class", string::highlight(this->building_class->get_name()));
	}

private:
	const metternich::building_class *building_class = nullptr;
};

}
