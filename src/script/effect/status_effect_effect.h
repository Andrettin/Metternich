#pragma once

#include "character/character.h"
#include "character/character_game_data.h"
#include "character/saving_throw_type.h"
#include "character/status_effect.h"
#include "script/effect/effect.h"
#include "util/dice.h"
#include "util/random.h"

namespace metternich {

class status_effect_effect final : public effect<const character>
{
public:
	explicit status_effect_effect(const std::string &value, const gsml_operator effect_operator)
		: effect(effect_operator)
	{
		this->status_effect = status_effect::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string identifier = "status_effect";
		return identifier;
	}

	virtual void do_assignment_effect(const character *scope, context &ctx) const override
	{
		bool apply_status_effect = true;

		if (this->status_effect->get_saving_throw_type() != nullptr) {
			const bool saving_throw_successful = scope->get_game_data()->do_saving_throw(this->status_effect->get_saving_throw_type(), this->status_effect->get_saving_throw_modifier());

			if (scope == game::get()->get_player_character()) {
				const portrait *war_minister_portrait = scope->get_game_data()->get_domain()->get_government()->get_war_minister_portrait();

				const std::string status_effect_adjective = !this->status_effect->get_adjective().empty() ? string::lowered(this->status_effect->get_adjective()) : std::format("affected by {}", this->status_effect->get_name());

				if (saving_throw_successful) {
					engine_interface::get()->add_notification("Saving Throw Successful!", war_minister_portrait, std::format("You have succeeded in a {} saving throw, and managed to avoid being {}!", this->status_effect->get_saving_throw_type()->get_name(), status_effect_adjective));
				} else {
					engine_interface::get()->add_notification("Saving Throw Failed!", war_minister_portrait, std::format("You have failed a {} saving throw, and are now {}!", this->status_effect->get_saving_throw_type()->get_name(), status_effect_adjective));
				}
			}

			apply_status_effect = !saving_throw_successful;
		}

		if (apply_status_effect) {
			std::optional<int> caster_level;

			if (std::holds_alternative<const character *>(ctx.source_scope)) {
				const character *caster = std::get<const character *>(ctx.source_scope);
				caster_level = caster->get_game_data()->get_level();
			}

			const std::chrono::seconds duration = this->status_effect->get_duration(caster_level);

			if (!ctx.in_combat && duration < std::chrono::months(1)) {
				//if we are out of combat and the duration is less than a month, resolve the status effect immediately
				if (this->status_effect->get_end_effects() != nullptr) {
					this->status_effect->get_end_effects()->do_effects(scope, ctx);
				}
			} else {
				scope->get_game_data()->set_status_effect_duration(this->status_effect, duration);
			}
		}
	}

	virtual std::string get_assignment_string(const character *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		const std::string_view status_effect_str = !this->status_effect->get_adjective().empty() ? this->status_effect->get_adjective() : this->status_effect->get_name();

		if (this->status_effect->get_saving_throw_type() != nullptr) {
			return std::format("{} (Saving Throw: {} {})", status_effect_str, this->status_effect->get_saving_throw_type()->get_name(), number::to_signed_string(scope->get_game_data()->get_saving_throw_bonus(this->status_effect->get_saving_throw_type()) + this->status_effect->get_saving_throw_modifier()));
		} else {
			return std::string(status_effect_str);
		}
	}

private:
	const metternich::status_effect *status_effect = nullptr;
};

}
