#pragma once

#include "map/terrain_type.h"
#include "script/condition/condition.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class terrain_condition final : public condition<scope_type>
{
public:
	explicit terrain_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->terrain = terrain_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "terrain";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		if (!scope->get_game_data()->is_on_map()) {
			return false;
		}

		return scope->get_game_data()->get_terrain() == this->terrain;
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("{} terrain", string::highlight(this->terrain->get_name()));
	}

private:
	const terrain_type *terrain = nullptr;
};

}
