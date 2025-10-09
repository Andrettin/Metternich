#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "item/enchantment.h"
#include "item/item.h"
#include "item/item_material.h"
#include "item/item_type.h"
#include "script/effect/effect.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
class lose_item_effect final : public effect<scope_type>
{
public:
	explicit lose_item_effect(const gsml_operator effect_operator)
		: effect<scope_type>(effect_operator)
	{
	}

	explicit lose_item_effect(const std::string &value, const gsml_operator effect_operator)
		: lose_item_effect(effect_operator)
	{
		this->type = item_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "lose_item";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "type") {
			this->type = item_type::get(value);
		} else if (key == "material") {
			this->material = item_material::get(value);
		} else if (key == "enchantment") {
			this->enchantment = enchantment::get(value);
		} else {
			effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		Q_UNUSED(ctx);

		const character *character = nullptr;

		if constexpr (std::is_same_v<scope_type, const metternich::character>) {
			character = scope;
		} else if constexpr (std::is_same_v<scope_type, const domain>) {
			character = scope->get_government()->get_ruler();
		} else {
			static_assert(false);
		}

		if (character == nullptr) {
			return;
		}

		character->get_game_data()->remove_item(this->type, this->material, this->enchantment);
	}

	virtual std::string get_assignment_string() const override
	{
		const std::string item_name = item::create_name(this->type, this->material, this->enchantment);

		return std::format("Lose {} {}", string::get_indefinite_article(item_name), item_name);
	}


private:
	const item_type *type = nullptr;
	const item_material *material = nullptr;
	const metternich::enchantment *enchantment = nullptr;
};

}
