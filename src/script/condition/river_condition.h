#pragma once

#include "map/site.h"
#include "map/site_map_data.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"

namespace metternich {

class river_condition final : public condition<site>
{
public:
	explicit river_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "river";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_map_data()->has_river();
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return "River";
		} else {
			return "No river";
		}
	}

private:
	bool value = false;
};

}
