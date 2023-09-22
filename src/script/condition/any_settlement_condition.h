#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

template <typename upper_scope_type>
class any_settlement_condition final : public scope_condition_base<upper_scope_type, site>
{
public:
	explicit any_settlement_condition(const gsml_operator condition_operator)
		: scope_condition_base<upper_scope_type, site>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_settlement";
		return class_identifier;
	}

	virtual bool check_assignment(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		if constexpr (std::is_same_v<upper_scope_type, country>) {
			for (const province *province : upper_scope->get_game_data()->get_provinces()) {
				for (const site *settlement_site : province->get_game_data()->get_settlement_sites()) {
					if (!settlement_site->get_game_data()->is_built()) {
						continue;
					}

					if (this->check_scope(settlement_site, ctx)) {
						return true;
					}
				}
			}
		} else if constexpr (std::is_same_v<upper_scope_type, province>) {
			for (const site *settlement_site : upper_scope->get_game_data()->get_settlement_sites()) {
				if (!settlement_site->get_game_data()->is_built()) {
					continue;
				}

				if (this->check_scope(settlement_site, ctx)) {
					return true;
				}
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any settlement";
	}
};

}
