#pragma once

#include "database/defines.h"
#include "game/character_event.h"
#include "game/country_event.h"
#include "game/game.h"
#include "game/province_event.h"
#include "script/effect/delayed_effect_instance.h"
#include "script/effect/effect.h"
#include "script/effect/effect_list.h"
#include "script/effect/scripted_effect.h"
#include "util/assert_util.h"
#include "util/random.h"

namespace metternich {

template <typename scope_type>
class scripted_effect_base;

template <typename scope_type>
class delayed_effect final : public effect<scope_type>
{
public:
	explicit delayed_effect(const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "delayed";
		return class_identifier;
	}

	virtual void process_gsml_property(const gsml_property &property) override
	{
		const std::string &key = property.get_key();
		const std::string &value = property.get_value();

		if (key == "scripted_effect") {
			if constexpr (std::is_same_v<scope_type, const character>) {
				this->scripted_effect = character_scripted_effect::get(value);
			} else if constexpr (std::is_same_v<scope_type, const country>) {
				this->scripted_effect = country_scripted_effect::get(value);
			} else if constexpr (std::is_same_v<scope_type, population_unit>) {
				this->scripted_effect = population_unit_scripted_effect::get(value);
			} else if constexpr (std::is_same_v<scope_type, const province>) {
				this->scripted_effect = province_scripted_effect::get(value);
			} else {
				assert_throw(false);
			}
		} else if (key == "event") {
			if constexpr (std::is_same_v<scope_type, const character>) {
				this->event = character_event::get(value);
			} else if constexpr (std::is_same_v<scope_type, const country>) {
				this->event = country_event::get(value);
			} else if constexpr (std::is_same_v<scope_type, const province>) {
				this->event = province_event::get(value);
			} else {
				assert_throw(false);
			}
		} else if (key == "delay") {
			this->delay = std::stoi(value);
		} else if (key == "delay_days") {
			this->delay_days = std::stoi(value);
		} else if (key == "random_delay") {
			this->random_delay = std::stoi(value);
		} else if (key == "random_delay_days") {
			this->random_delay_days = std::stoi(value);
		} else {
			effect<scope_type>::process_gsml_property(property);
		}
	}

	virtual void check() const override
	{
		if (this->scripted_effect == nullptr && this->event == nullptr) {
			throw std::runtime_error("\"" + this->get_class_identifier() + "\" effect has neither a scripted effect nor an event set for it.");
		}
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		const int current_year = game::get()->get_year();

		int delay = this->delay;
		if (this->delay_days > 0) {
			delay += defines::get()->days_to_turns(this->delay_days, current_year).to_int();
		}
		if (this->random_delay > 0) {
			delay += random::get()->generate_in_range(0, this->random_delay);
		}
		if (this->random_delay_days > 0) {
			delay += random::get()->generate_in_range(0, defines::get()->days_to_turns(this->random_delay_days, current_year).to_int());
		}

		delay = std::max(1, delay);

		std::unique_ptr<delayed_effect_instance<scope_type>> delayed_effect;
		if (this->scripted_effect != nullptr) {
			delayed_effect = std::make_unique<delayed_effect_instance<scope_type>>(this->scripted_effect, scope, ctx, delay);
		} else {
			delayed_effect = std::make_unique<delayed_effect_instance<scope_type>>(this->event, scope, ctx, delay);
		}

		game::get()->add_delayed_effect(std::move(delayed_effect));
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(prefix);

		std::string str;

		if (this->delay > 0 || this->random_delay > 0) {
			str = "In ";
			if (this->delay > 0 || this->delay_days > 0) {
				if (this->random_delay > 0 || this->random_delay_days > 0) {
					str += "around ";
				}
				str += std::to_string(this->delay + defines::get()->days_to_turns(this->delay_days, game::get()->get_year()).to_int());
			} else if (this->random_delay > 0 || this->random_delay_days > 0) {
				str += "some";
			}
			str += " turns:\n";
		}

		const size_t new_indent = str.empty() ? indent : (indent + 1);

		if (this->scripted_effect != nullptr) {
			str += this->scripted_effect->get_effects().get_effects_string(scope, ctx, new_indent, prefix);
		} else {
			if (!str.empty()) {
				str += std::string(new_indent, '\t');
			}

			str += "Trigger the " + this->event->get_name() + " event";
		}

		return str;
	}

private:
	const scripted_effect_base<scope_type> *scripted_effect = nullptr;
	const scoped_event_base<scope_type> *event = nullptr;
	int delay = 0;
	int delay_days = 0;
	int random_delay = 0;
	int random_delay_days = 0;
};

}
