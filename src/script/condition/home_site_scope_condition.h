#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "map/site.h"
#include "map/site_map_data.h"
#include "script/condition/scope_condition.h"

namespace metternich {

class site;

class home_site_scope_condition final : public scope_condition<character, site, read_only_context, condition<site>>
{
public:
	explicit home_site_scope_condition(const gsml_operator condition_operator)
		: scope_condition<character, site, read_only_context, condition<site>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "home_site_scope";
		return class_identifier;
	}

	virtual const site *get_scope(const character *upper_scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (upper_scope->get_game_data()->get_home_site() != nullptr && !upper_scope->get_game_data()->get_home_site()->get_map_data()->is_on_map()) {
			return nullptr;
		}

		return upper_scope->get_game_data()->get_home_site();
	}

	virtual std::string get_scope_name() const override
	{
		return "Home Site";
	}
};

}
