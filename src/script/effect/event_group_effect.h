#pragma once

#include "game/event_random_group.h"
#include "game/event_trigger.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class event_group_effect final : public effect<scope_type>
{
public:
	explicit event_group_effect(const std::string &value, const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
		if (value == "random") {
			this->random_event = true;
		} else {
			this->event_group = event_random_group::get(value);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "event_group";
		return class_identifier;
	}

	virtual void check() const override
	{
		if (this->event_group == nullptr && !this->random_event) {
			throw std::runtime_error(std::format("\"{}\" effect has no event group set for it, and it also does not trigger a random event.", this->get_class_identifier()));
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(scope_type *scope, context &ctx) const override
	{
		context event_ctx = ctx;
		event_ctx.root_scope = scope;
		event_ctx.source_scope = ctx.root_scope;

		const auto &events = this->event_group != nullptr ? this->event_group->get_events<scope_type>() : scoped_event_base<scope_type>::get_trigger_random_events(event_trigger::per_turn_pulse);

		co_await scoped_event_base<scope_type>::check_random_events_for_scope(scope, event_ctx, events, this->event_group != nullptr ? this->event_group->get_delay(game::get()->get_year()) : 0);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		if (this->random_event) {
			return "Trigger a random event";
		}

		return std::format("Trigger {} {} event", string::get_indefinite_article(this->event_group->get_name()), this->event_group->get_name());
	}

private:
	bool random_event = false;
	const event_random_group *event_group = nullptr;
};

}
