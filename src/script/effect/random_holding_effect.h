#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "map/map.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/and_condition.h"
#include "script/effect/scope_effect_base.h"
#include "util/string_conversion_util.h"
#include "util/vector_random_util.h"

namespace metternich {

class random_holding_effect final : public scope_effect_base<const domain, const site>
{
public:
	explicit random_holding_effect(const gsml_operator effect_operator)
		: scope_effect_base<const domain, const site>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "random_holding";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "global") {
			this->global = string::to_bool(value);
		} else if (key == "built_only") {
			this->built_only = string::to_bool(value);
		} else {
			scope_effect_base<const domain, const site>::process_gsml_property(property);
		}
	}

	virtual void process_gsml_scope(const gsml_data &scope) override
	{
		const std::string &tag = scope.get_tag();

		if (tag == "conditions") {
			this->conditions.process_gsml_data(scope);
		} else {
			scope_effect_base<const domain, const site>::process_gsml_scope(scope);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(const domain *upper_scope, context &ctx) const override
	{
		std::vector<const site *> potential_holdings;

		const std::vector<const site *> &scope_sites = this->global ? map::get()->get_sites() : upper_scope->get_game_data()->get_sites();

		for (const site *holding : scope_sites) {
			if (!holding->is_settlement()) {
				continue;
			}

			if (this->built_only && !holding->get_game_data()->is_built()) {
				continue;
			}

			if (!this->conditions.check(holding, ctx)) {
				continue;
			}

			potential_holdings.push_back(holding);
		}

		if (!potential_holdings.empty()) {
			co_await this->do_scope_effect(vector::get_random(potential_holdings), upper_scope, ctx);
		}
	}

	virtual std::string get_scope_name() const override
	{
		return "Random holding";
	}

	virtual std::string get_conditions_string(const size_t indent) const override
	{
		return this->conditions.get_conditions_string(indent);
	}

private:
	bool global = false; //whether to check *all* holdings on the map
	bool built_only = true; //whether to check only built holdings
	and_condition<site> conditions;
};

}
