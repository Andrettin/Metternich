#include "metternich.h"

#include "script/battle_enemy.h"

#include "character/monster_type.h"
#include "database/gsml_data.h"
#include "game/combat_base.h"
#include "item/item_type.h"
#include "script/effect/effect_list.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

battle_enemy::battle_enemy() : placement(combat_placement::right)
{
}

battle_enemy::battle_enemy(const gsml_data &scope) : battle_enemy()
{
	this->military_unit_type = military_unit_type::get(scope.get_tag());

	scope.process(this);
}

battle_enemy::~battle_enemy()
{
}

void battle_enemy::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "military_unit_type") {
		assert_throw(this->military_unit_type == nullptr);
		this->military_unit_type = military_unit_type::get(value);
	} else if (key == "monster_type") {
		this->monster_type = monster_type::get(value);
	} else if (key == "health") {
		this->health = std::stoi(value);
	} else if (key == "placement") {
		this->placement = magic_enum::enum_cast<combat_placement>(value).value();
	} else {
		assert_throw(false);
	}
}

void battle_enemy::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "placement_offset") {
		this->placement_offset = scope.to_point();
	} else if (tag == "items") {
		for (const std::string &value : values) {
			this->items.push_back(item_type::get(value));
		}

		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const item_type *item_type = item_type::get(key);
			const int quantity = std::stoi(value);

			for (int i = 0; i < quantity; ++i) {
				this->items.push_back(item_type);
			}
		});
	} else if (tag == "on_killed") {
		this->kill_effects = std::make_unique<effect_list<const domain>>();
		this->kill_effects->process_gsml_data(scope);
	} else {
		assert_throw(false);
	}
}

}
