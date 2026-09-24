#pragma once

#include "map/site.h"
#include "map/site_game_data.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

class explore_dungeon_effect final : public effect<const domain>
{
public:
	explicit explore_dungeon_effect(const bool value, const gsml_operator effect_operator)
		: effect(effect_operator)
	{
		this->value = value;
	}

	explicit explore_dungeon_effect(const std::string &value, const gsml_operator effect_operator)
		: explore_dungeon_effect(string::to_bool(value), effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "explore_dungeon";
		return class_identifier;
	}

	[[nodiscard]]
	virtual QCoro::Task<void> do_assignment_effect_coro(const domain *scope, context &ctx) const override
	{
		assert_throw(ctx.attacking_army != nullptr);
		assert_throw(ctx.attacking_army->get_domain() == scope);

		assert_throw(ctx.ruin_site != nullptr);

		if (this->value) {
			co_await ctx.ruin_site->get_game_data()->explore_ruin(ctx.attacking_army);
		}
	}

	virtual std::string get_assignment_string(const domain *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		if (this->value) {
			assert_throw(ctx.ruin_site != nullptr);

			if (ctx.ruin_site->get_game_data()->is_ruin()) {
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
