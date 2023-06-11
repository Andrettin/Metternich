#pragma once

#include "country/country.h"
#include "country/country_game_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/terrain_type.h"
#include "script/condition/condition.h"

namespace metternich {

template <typename scope_type>
class has_terrain_condition final : public condition<scope_type>
{
public:
	explicit has_terrain_condition(const std::string &value, const gsml_operator condition_operator)
		: condition<scope_type>(condition_operator)
	{
		this->terrain = terrain_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "has_terrain";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(ctx);

		return scope->get_game_data()->get_tile_terrain_counts().contains(this->terrain);
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return "Has " + this->terrain->get_name() + " terrain";
	}

private:
	const terrain_type *terrain = nullptr;
};

}
