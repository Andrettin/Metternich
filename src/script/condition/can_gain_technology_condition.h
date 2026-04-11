#pragma once

#include "script/condition/condition.h"
#include "technology/technology.h"

namespace metternich {

template <typename scope_type>
class can_gain_technology_condition final : public condition<scope_type>
{
public:
	explicit can_gain_technology_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->technology = technology::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "can_gain_building_class";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if constexpr (std::is_same_v<scope_type, domain>) {
			return scope->get_technology()->can_gain_technology(this->technology);
		} else {
			return scope->get_game_data()->can_gain_technology(this->technology);
		}
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("Can gain the {} technology", string::highlight(this->technology->get_name()));
	}

private:
	const metternich::technology *technology = nullptr;
};

}
