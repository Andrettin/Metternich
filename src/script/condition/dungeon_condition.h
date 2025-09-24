#pragma once

#include "infrastructure/dungeon.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class dungeon_condition final : public condition<scope_type>
{
public:
	explicit dungeon_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->dungeon = dungeon::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "dungeon";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		if constexpr (std::is_same_v<scope_type, site>) {
			return scope->get_game_data()->get_dungeon() == this->dungeon;
		} else {
			if (ctx.dungeon_site != nullptr) {
				return ctx.dungeon_site->get_game_data()->get_dungeon() == this->dungeon;
			}

			return false;
		}
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} dungeon", this->dungeon->get_name());
	}

private:
	const metternich::dungeon *dungeon = nullptr;
};

}
