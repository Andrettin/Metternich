#pragma once

#include "map/site.h"
#include "map/site_map_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace metternich {

class adjacent_terrain_condition final : public condition<site>
{
public:
	explicit adjacent_terrain_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<site>(condition_operator)
	{
		this->terrain = terrain_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "adjacent_terrain";
		return class_identifier;
	}

	virtual bool check_assignment(const site *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (!scope->get_map_data()->is_on_map()) {
			return false;
		}

		return scope->get_map_data()->get_adjacent_terrain_count(this->terrain) > 0;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} adjacent terrain", string::highlight(this->terrain->get_name()));
	}

private:
	const terrain_type *terrain = nullptr;
};

}
