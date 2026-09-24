#pragma once

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
		this->value = string::to_bool(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "dungeon";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		if constexpr (std::is_same_v<scope_type, site>) {
			return scope->get_game_data()->is_ruin() == this->value;
		} else {
			if (ctx.ruin_site != nullptr) {
				return ctx.ruin_site->get_game_data()->is_ruin() == this->value;
			}

			return false;
		}
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Ruin";
	}

private:
	bool value = false;
};

}
