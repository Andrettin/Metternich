#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace metternich {

class terrain_condition final : public condition<site>
{
public:
	explicit terrain_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->terrain = terrain_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "terrain";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (!scope->get_game_data()->is_on_map()) {
			return false;
		}

		return scope->get_game_data()->get_tile()->get_terrain() == this->terrain;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return string::highlight(this->terrain->get_name()) + " terrain";
	}

private:
	const terrain_type *terrain = nullptr;
};

}
