#pragma once

#include "country/country.h"
#include "country/government_type.h"
#include "script/condition/condition.h"

namespace metternich {

class government_type_condition final : public condition<country>
{
public:
	explicit government_type_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<country>(condition_operator)
	{
		this->government_type = government_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "government_type";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_government_type() == this->government_type;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} government type", string::highlight(this->government_type->get_name()));
	}

private:
	const metternich::government_type *government_type = nullptr;
};

}
