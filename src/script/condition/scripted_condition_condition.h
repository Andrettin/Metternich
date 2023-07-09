#pragma once

#include "script/condition/condition.h"
#include "script/condition/scripted_condition.h"

namespace metternich {

template <typename scope_type>
class scripted_condition_condition final : public condition<scope_type>
{
public:
	static const scripted_condition_base<scope_type> *get_scripted_condition(const std::string &identifier)
	{
		if constexpr (std::is_same_v<scope_type, character>) {
			return character_scripted_condition::get(identifier);
		} else if constexpr (std::is_same_v<scope_type, country>) {
			return country_scripted_condition::get(identifier);
		} else if constexpr (std::is_same_v<scope_type, military_unit>) {
			return military_unit_scripted_condition::get(identifier);
		} else if constexpr (std::is_same_v<scope_type, population_unit>) {
			return population_unit_scripted_condition::get(identifier);
		} else if constexpr (std::is_same_v<scope_type, province>) {
			return province_scripted_condition::get(identifier);
		} else if constexpr (std::is_same_v<scope_type, site>) {
			return site_scripted_condition::get(identifier);
		}
	}

	explicit scripted_condition_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->scripted_condition = scripted_condition_condition::get_scripted_condition(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "scripted_condition";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		return this->scripted_condition->get_conditions()->check(scope, ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		return this->scripted_condition->get_conditions()->get_string(indent);
	}

private:
	const scripted_condition_base<scope_type> *scripted_condition = nullptr;
};

}
