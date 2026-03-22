#pragma once

#include "database/database.h"
#include "script/effect/effect.h"
#include "script/flag.h"
#include "util/string_util.h"

namespace metternich {

class flag;

template <typename scope_type>
class clear_flag_effect final : public effect<scope_type>
{
public:
	explicit clear_flag_effect(const std::string &flag_identifier, const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
		this->flag = flag::get_or_add(flag_identifier, database::get()->get_current_module());
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "clear_flag";
		return class_identifier;
	}

	virtual void do_assignment_effect(scope_type *scope) const override
	{
		scope->get_game_data()->clear_flag(this->flag);
	}

	virtual std::string get_assignment_string() const override
	{
		return std::format("Clear the {} flag", string::highlight(this->flag->get_name()));
	}

	virtual bool is_hidden() const override
	{
		return true;
	}

private:
	const metternich::flag *flag = nullptr;
};

}
