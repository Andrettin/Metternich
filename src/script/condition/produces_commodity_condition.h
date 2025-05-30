#pragma once

#include "economy/commodity.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class produces_commodity_condition final : public condition<scope_type>
{
public:
	explicit produces_commodity_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->commodity = commodity::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "produces_commodity";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->produces_commodity(this->commodity);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Produces " + string::highlight(this->commodity->get_name());
	}

private:
	metternich::commodity *commodity = nullptr;
};

}
