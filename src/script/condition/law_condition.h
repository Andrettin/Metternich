#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/law.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class law_condition final : public condition<scope_type>
{
public:
	explicit law_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->law = law::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "law";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		const country *country = condition<scope_type>::get_scope_country(scope);

		if (country == nullptr) {
			return false;
		}

		return country->get_game_data()->has_law(this->law);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} law", string::highlight(this->law->get_name()));
	}

private:
	const metternich::law *law = nullptr;
};

}
