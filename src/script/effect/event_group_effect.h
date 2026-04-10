#pragma once

#include "game/event_random_group.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class event_group_effect final : public effect<scope_type>
{
public:
	explicit event_group_effect(const std::string &value, const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
		this->event_group = event_random_group::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "event_group";
		return class_identifier;
	}

	virtual void check() const override
	{
		if (this->event_group == nullptr) {
			throw std::runtime_error(std::format("\"{}\" effect has no event group set for it.", this->get_class_identifier()));
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(scope_type *scope, context &ctx) const override
	{
		context event_ctx = ctx;
		event_ctx.root_scope = scope;
		event_ctx.source_scope = ctx.root_scope;

		co_await scoped_event_base<scope_type>::check_random_events_for_scope(scope, event_ctx, this->event_group->get_events<scope_type>(), this->event_group->get_delay(game::get()->get_year()));
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		return std::format("Trigger {} {} event", string::get_indefinite_article(this->event_group->get_name()), this->event_group->get_name());
	}

private:
	const event_random_group *event_group = nullptr;
};

}
