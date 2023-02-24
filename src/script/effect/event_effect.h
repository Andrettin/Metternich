#pragma once

#include "game/character_event.h"
#include "game/country_event.h"
#include "game/province_event.h"
#include "script/effect/effect.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class event_effect final : public effect<scope_type>
{
public:
	explicit event_effect(const std::string &value, const gsml_operator effect_operator) : effect<scope_type>(effect_operator)
	{
		if constexpr (std::is_same_v<scope_type, const character>) {
			this->event = character_event::get(value);
		} else if constexpr (std::is_same_v<scope_type, const country>) {
			this->event = country_event::get(value);
		} else if constexpr (std::is_same_v<scope_type, const province>) {
			this->event = province_event::get(value);
		} else {
			assert_throw(false);
		}
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "event";
		return class_identifier;
	}

	virtual void check() const override
	{
		if (this->event == nullptr) {
			throw std::runtime_error("\"" + this->get_class_identifier() + "\" effect has no event set for it.");
		}
	}

	virtual void do_assignment_effect(scope_type *scope, context &ctx) const override
	{
		context event_ctx = ctx;
		event_ctx.root_scope = scope;
		event_ctx.source_scope = ctx.root_scope;

		this->event->fire(scope, event_ctx);
	}

	virtual std::string get_assignment_string(const scope_type *scope, const read_only_context &ctx, const size_t indent, const std::string &prefix) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);
		Q_UNUSED(indent);
		Q_UNUSED(prefix);

		return "Trigger the " + this->event->get_name() + " event";
	}

private:
	const scoped_event_base<scope_type> *event = nullptr;
};

}
