#pragma once

#include "script/condition/condition.h"
#include "unit/military_unit.h"
#include "unit/promotion.h"
#include "util/string_util.h"

namespace metternich {

class promotion_condition final : public condition<military_unit>
{
public:
	explicit promotion_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<military_unit>(condition_operator)
	{
		this->promotion = promotion::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "promotion";
		return class_identifier;
	}

	virtual bool check_assignment(const military_unit *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->has_promotion(this->promotion);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight(this->promotion->get_name());
	}

private:
	const metternich::promotion *promotion = nullptr;
};

}
