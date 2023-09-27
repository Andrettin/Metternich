#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/database.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"
#include "util/vector_random_util.h"

namespace metternich {

class country;

class random_known_country_effect final : public scope_effect_base<const country, const country>
{
public:
	explicit random_known_country_effect(const gsml_operator effect_operator)
		: scope_effect_base<const country, const country>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random_known_country";
		return class_identifier;
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			database::process_gsml_data(this->conditions, scope);
		} else {
			scope_effect_base<const country, const country>::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(const country *upper_scope, context &ctx) const override
	{
		std::vector<const country *> potential_countries;

		for (const country *known_country : upper_scope->get_game_data()->get_known_countries()) {
			if (!this->conditions.check(known_country, ctx)) {
				continue;
			}

			potential_countries.push_back(known_country);
		}

		if (!potential_countries.empty()) {
			this->do_scope_effect(vector::get_random(potential_countries), upper_scope, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Random known country";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	and_condition<country> conditions;
};

}
