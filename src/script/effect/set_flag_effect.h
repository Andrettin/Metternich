#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/effect/effect.h"
#include "script/flag.h"
#include "util/string_util.h"

namespace metternich {

class flag;

class set_flag_effect final : public effect<const country>
{
public:
	explicit set_flag_effect(const metternich::flag *flag, const gsml_operator effect_operator)
		: effect(effect_operator), flag(flag)
	{
	}

	explicit set_flag_effect(const metternich::flag *flag)
		: set_flag_effect(flag, gsml_operator::assignment)
	{
	}

	explicit set_flag_effect(const std::string &flag_identifier, const gsml_operator effect_operator)
		: set_flag_effect(flag::get_or_add(flag_identifier, database::get()->get_current_module()), effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "set_flag";
		return class_identifier;
	}

	virtual void do_assignment_effect(const country *scope) const override
	{
		scope->get_game_data()->set_flag(this->flag);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Set the {} flag", string::highlight(this->flag->get_name()));
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	const metternich::flag *flag = nullptr;
};

}
