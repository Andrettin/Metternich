#pragma once

#include "character/party.h"
#include "infrastructure/dungeon_area.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

class explore_dungeon_effect final : public effect<const domain>
{
public:
	explicit explore_dungeon_effect(const std::string &value, const gsml_operator effect_operator)
		: effect(effect_operator)
	{
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "explore_dungeon";
		return class_identifier;
	}

	virtual void do_assignment_effect(const domain *scope, context &ctx) const override
	{
		assert_throw(ctx.party != nullptr);
		assert_throw(ctx.party->get_domain() == scope);

		assert_throw(ctx.dungeon_site != nullptr);

		if (ctx.dungeon_area != nullptr) {
			ctx.dungeon_site->get_game_data()->add_explored_dungeon_area(ctx.dungeon_area);
		}

		if (this->value) {
			ctx.dungeon_site->get_game_data()->explore_dungeon(ctx.party);
		}
	}

	virtual std::string get_assignment_string(const domain *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		if (this->value) {
			assert_throw(ctx.dungeon_site != nullptr);

			std::vector<const dungeon_area *> potential_dungeon_areas = ctx.dungeon_site->get_game_data()->get_potential_dungeon_areas();
			if (ctx.dungeon_area != nullptr && !ctx.dungeon_area->is_entrance()) {
				std::erase_if(potential_dungeon_areas, [&ctx](const dungeon_area *dungeon_area) {
					return dungeon_area == ctx.dungeon_area;
				});
			}

			if (!potential_dungeon_areas.empty()) {
				return "Explore the dungeon further";
			} else {
				return "Exit the dungeon";
			}
		} else {
			return "Retreat from the dungeon";
		}
	}

private:
	bool value = false;
};

}
