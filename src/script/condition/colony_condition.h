#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "script/condition/condition.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class colony_condition final : public condition<scope_type>
{
public:
	explicit colony_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "colony";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const country *country = condition<scope_type>::get_scope_country(scope);

		if (country == nullptr) {
			return false;
		}

		return country->get_game_data()->is_colony() == this->value;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		if (this->value) {
			return string::highlight("Colony");
		} else {
			return "Not a " + string::highlight("Colony");
		}
	}

private:
	bool value = false;
};

}
