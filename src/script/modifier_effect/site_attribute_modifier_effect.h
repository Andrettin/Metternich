#pragma once

#include "map/site.h"
#include "map/site_attribute.h"
#include "map/site_game_data.h"
#include "script/modifier_effect/modifier_effect.h"
#include "util/dice.h"
#include "util/random.h"

namespace metternich {

template <typename scope_type>
class site_attribute_modifier_effect final : public modifier_effect<scope_type>
{
public:
	explicit site_attribute_modifier_effect(const site_attribute *attribute, const std::string &value)
		: modifier_effect<scope_type>(value), attribute(attribute)
	{
		if (string::is_number(value)) {
			this->value = centesimal_int(value);
		} else {
			this->value_dice = dice(value);
		}
	}

	virtual const std::string &get_identifier() const override
	{
		static const std::string identifier = "site_attribute";
		return identifier;
	}

	[[nodiscard]] virtual QCoro::Task<void> apply_coro(const scope_type *scope, const centesimal_int &multiplier) const override
	{
		if constexpr (std::is_same_v<scope_type, const site>) {
			if (!this->value_dice.is_null()) {
				if (multiplier == 1) {
					const int roll_result = random::get()->roll_dice(this->value_dice);
					co_await scope->get_game_data()->add_attribute_roll_result(this->attribute, this->value_dice, roll_result);
				} else if (multiplier == -1) {
					co_await scope->get_game_data()->remove_attribute_roll_result(this->attribute, this->value_dice);
				} else {
					assert_throw(false);
				}
			} else {
				co_await scope->get_game_data()->change_attribute_value(this->attribute, (this->value * multiplier).to_int());
			}
		} else {
			assert_throw(this->value_dice.is_null());
			co_await scope->get_game_data()->change_site_attribute_value(this->attribute, (this->value * multiplier).to_int());
		}
	}

	virtual std::string get_base_string(const scope_type *scope) const override
	{
		Q_UNUSED(scope);

		return this->attribute->get_name();
	}

	virtual std::string get_number_string(const centesimal_int &multiplier, const bool ignore_decimals) const override
	{
		if (!this->value_dice.is_null()) {
			return (multiplier > 0 ? "+" : "-") + this->value_dice.to_display_string();
		} else {
			return modifier_effect<scope_type>::get_number_string(multiplier, ignore_decimals);
		}
	}

	virtual bool is_negative(const centesimal_int &multiplier) const override
	{
		if (!this->value_dice.is_null()) {
			return multiplier < 0;
		} else {
			return modifier_effect<scope_type>::is_negative(multiplier);
		}
	}

private:
	const site_attribute *attribute = nullptr;
	dice value_dice;
};

}
