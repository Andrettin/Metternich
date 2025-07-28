#pragma once

#include "map/map.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/tile.h"
#include "script/condition/scope_condition_base.h"
#include "script/context.h"
#include "util/point_util.h"

namespace metternich {

class any_adjacent_site_condition final : public scope_condition_base<site, site, read_only_context, condition<site>>
{
public:
	explicit any_adjacent_site_condition(const gsml_operator condition_operator)
		: scope_condition_base<site, site, read_only_context, condition<site>>(condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "any_adjacent_site";
		return class_identifier;
	}

	virtual bool check_assignment(const site *upper_scope, const read_only_context &ctx) const override
	{
		bool result = false;

		point::for_each_adjacent_until(upper_scope->get_map_data()->get_tile_pos(), [this, upper_scope, &ctx, &result](const QPoint &adjacent_pos) {
			if (!map::get()->contains(adjacent_pos)) {
				return false;
			}

			const metternich::tile *adjacent_tile = map::get()->get_tile(adjacent_pos);
			const site *adjacent_site = adjacent_tile->get_site();

			if (adjacent_site == nullptr) {
				return false;
			}

			//only consider adjacent sites belonging to the same owner
			if (adjacent_site->get_game_data()->get_owner() != upper_scope->get_game_data()->get_owner()) {
				return false;
			}

			if (this->check_scope(adjacent_site, ctx)) {
				result = true;
				return true;
			}

			return false;
		});

		return result;
	}

	virtual std::string get_scope_name() const override
	{
		return "Any adjacent site";
	}
};

}
