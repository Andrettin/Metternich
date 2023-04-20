#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "map/province.h"
#include "script/condition/condition.h"

namespace metternich {

class discovered_province_condition final : public condition<country>
{
public:
	explicit discovered_province_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<country>(condition_operator)
	{
		this->province = province::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "discovered_province";
		return class_identifier;
	}

	virtual bool check_assignment(const country *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->is_province_discovered(this->province);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Discovered the " + this->province->get_game_data()->get_current_cultural_name() + " province";
	}

private:
	const metternich::province *province = nullptr;
};

}
