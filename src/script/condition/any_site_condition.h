#pragma once

#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/scope_condition_base.h"

namespace metternich {

template <typename upper_scope_type>
class any_site_condition final : public scope_condition_base<upper_scope_type, site, read_only_context, condition<site>>
{
public:
	explicit any_site_condition(const gsml_operator condition_operator)
		: scope_condition_base<upper_scope_type, site, read_only_context, condition<site>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_site";
		return class_identifier;
	}

	virtual bool check_assignment(const upper_scope_type *upper_scope, const read_only_context &ctx) const override
	{
		if constexpr (std::is_same_v<upper_scope_type, domain>) {
			for (const site *site : upper_scope->get_game_data()->get_sites()) {
				if (this->check_scope(site, ctx)) {
					return true;
				}
			}
		} else if constexpr (std::is_same_v<upper_scope_type, province>) {
			for (const site *site : upper_scope->get_game_data()->get_sites()) {
				if (this->check_scope(site, ctx)) {
					return true;
				}
			}
		}

		return false;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any site";
	}
};

}
