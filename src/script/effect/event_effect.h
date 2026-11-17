#pragma once

#include "game/character_event.h"
#include "game/domain_event.h"
#include "game/province_event.h"
#include "game/site_event.h"
#include "script/context.h"
#include "script/effect/effect.h"
#include "script/scope_traits.h"

namespace metternich {

template <typename scope_type>
class event_effect final : public effect<scope_type>
{
public:
	explicit event_effect(const std::string &value, const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
		this->event = scope_traits_t<scope_type>::event_type::get(value);
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "event";
		return class_identifier;
	}

	virtual void check() const override
	{
		if (this->event == nullptr) {
			throw std::runtime_error(std::format("\"{}\" effect has no event set for it.", this->get_class_identifier()));
		}
	}

	[[nodiscard]] virtual QCoro::Task<void> do_assignment_effect_coro(scope_type *scope, context &ctx) const override
	{
		context event_ctx = ctx;
		event_ctx.root_scope = scope;
		event_ctx.source_scope = ctx.root_scope;

		co_await this->event->fire(scope, event_ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		return std::format("Trigger the {} event", this->event->get_name());
	}

private:
	const scoped_event_base<scope_type> *event = nullptr;
};

}
