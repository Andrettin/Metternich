#pragma once

#include "infrastructure/settlement_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

class settlement_type_condition final : public condition<site>
{
public:
	explicit settlement_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->settlement_type = settlement_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "settlement_type";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_settlement_type() == this->settlement_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} settlement type", this->settlement_type->get_name());
	}

private:
	const metternich::settlement_type *settlement_type = nullptr;
};

}
