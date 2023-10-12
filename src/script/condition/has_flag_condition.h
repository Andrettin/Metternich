#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/condition.h"
#include "script/flag.h"
#include "util/string_util.h"

namespace metternich {

class flag;

class has_flag_condition final : public condition<country>
{
public:
	explicit has_flag_condition(const metternich::flag *flag)
		: condition(gsml_operator::assignment), flag(flag)
	{
	}

	explicit has_flag_condition(const std::string &value, const gsml_operator condition_operator)
		: condition(condition_operator), flag(flag::get_or_add(value, database::get()->get_current_module()))
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_flag";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->has_flag(this->flag);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("Has the {} flag", string::highlight(this->flag->get_name()));
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	const metternich::flag *flag = nullptr;
};

}
