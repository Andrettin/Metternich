#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/database.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"
#include "util/vector_random_util.h"

namespace metternich {

class random_settlement_effect final : public scope_effect_base<const country, const site>
{
public:
	explicit random_settlement_effect(const gsml_operator effect_operator)
		: scope_effect_base<const country, const site>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random_settlement";
		return class_identifier;
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			database::process_gsml_data(this->conditions, scope);
		} else {
			scope_effect_base<const country, const site>::process_gsml_scope(scope);
		}
	}

	virtual void do_assignment_effect(const country *upper_scope, context &ctx) const override
	{
		std::vector<const site *> potential_settlements;

		for (const province *province : upper_scope->get_game_data()->get_provinces()) {
			for (const site *settlement : province->get_game_data()->get_settlement_sites()) {
				if (!settlement->get_game_data()->is_built()) {
					continue;
				}

				if (!this->conditions.check(settlement, ctx)) {
					continue;
				}

				potential_settlements.push_back(settlement);
			}
		}

		if (!potential_settlements.empty()) {
			this->do_scope_effect(vector::get_random(potential_settlements), upper_scope, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Random settlement";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	and_condition<site> conditions;
};

}
