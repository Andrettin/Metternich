#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "item/item_type.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/assert_util.h"

namespace metternich {

class natural_weapon_modifier_effect final : public modifier_effect<const character>
{
public:
	natural_weapon_modifier_effect() = default;

	explicit natural_weapon_modifier_effect(const std::string &value)
	{
		this->natural_weapon_type = item_type::get(value);
		this->value = decimillesimal_int(1);
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "natural_weapon";
		return identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "type") {
			this->natural_weapon_type = item_type::get(value);
		} else if (key == "count") {
			this->value = decimillesimal_int(std::stoi(value));
		} else {
			modifier_effect::process_gsml_property(property);
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(const character *scope, const decimillesimal_int &multiplier) const override
	{
		assert_throw(this->natural_weapon_type->get_item_class()->is_natural_weapon());

		const int change = (this->value * multiplier).to_int();

		if (change == 0) {
			co_return;
		}

		if (change > 0) {
			for (int i = 0; i < change; ++i) {
				auto item = make_qunique<metternich::item>(this->natural_weapon_type, nullptr, nullptr, nullptr, nullptr);
				co_await scope->get_game_data()->add_item(std::move(item));
			}
		} else {
			const int abs_change = std::abs(change);
			for (int i = 0; i < abs_change; ++i) {
				assert_throw(scope->get_game_data()->has_item(this->natural_weapon_type));
				co_await scope->get_game_data()->remove_item(this->natural_weapon_type, nullptr, nullptr, nullptr, nullptr);
			}
		}
	}

	virtual std::string get_base_string(const character *scope) const override
	{
		Q_UNUSED(scope);

		return "Natural Weapon";
	}

	virtual std::string get_string(const character *scope, const decimillesimal_int &multiplier, const size_t indent, const bool ignore_decimals, const std::string &separator) const override
	{
		Q_UNUSED(indent);
		Q_UNUSED(ignore_decimals);
		Q_UNUSED(separator);

		return std::format("{} {}: {}", (this->value * multiplier) > 0 ? "Gain" : "Lose", this->get_base_string(scope), this->natural_weapon_type->get_name());
	}

private:
	const metternich::item_type *natural_weapon_type = nullptr;
};

}
