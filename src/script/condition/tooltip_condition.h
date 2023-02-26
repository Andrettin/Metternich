#pragma once

#include "script/condition/and_condition.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class tooltip_condition final : public condition<scope_type>
{
public:
	explicit tooltip_condition(const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		if (property.get_key() == "text") {
			this->text = property.get_value();
		} else {
			this->conditions.process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		this->conditions.process_gsml_scope(scope);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "tooltip";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		return this->conditions.check_assignment(scope, ctx);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return this->text;
	}

private:
	std::string text;
	and_condition<scope_type> conditions;
};

}
