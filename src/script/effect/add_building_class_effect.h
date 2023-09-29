#pragma once

#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

class add_building_class_effect final : public effect<const site>
{
public:
	explicit add_building_class_effect(const std::string &value, const gsml_operator effect_operator)
		: effect<const site>(effect_operator)
	{
		this->building_class = building_class::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "add_building_class";
		return class_identifier;
	}

	virtual void do_assignment_effect(const site *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		const building_type *building = scope->get_game_data()->get_building_class_type(this->building_class);

		if (building == nullptr) {
			return;
		}

		if (!scope->get_game_data()->can_gain_building(building)) {
			return;
		}

		scope->get_game_data()->add_building(building);
	}

	virtual std::string get_assignment_string(const site *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		const building_type *building = scope->get_game_data()->get_building_class_type(this->building_class);

		if (building == nullptr) {
			return std::string();
		}

		return std::format("Gain {} building", string::highlight(building->get_name()));
	}

private:
	const metternich::building_class *building_class = nullptr;
};

}
