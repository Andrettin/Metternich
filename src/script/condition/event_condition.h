#pragma once

#include "game/character_event.h"
#include "game/domain_event.h"
#include "game/province_event.h"
#include "game/site_event.h"
#include "script/condition/condition.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
class event_condition final : public condition<scope_type>
{
public:
	static const scoped_event_base<const scope_type> *get_event(const std::string &value)
	{
		if constexpr (std::is_same_v<scope_type, character>) {
			return character_event::get(value);
		} else if constexpr (std::is_same_v<scope_type, domain>) {
			return domain_event::get(value);
		} else if constexpr (std::is_same_v<scope_type, province>) {
			return province_event::get(value);
		} else if constexpr (std::is_same_v<scope_type, site>) {
			return site_event::get(value);
		} else {
			assert_throw(false);
			return nullptr;
		}
	}

	explicit event_condition(const scoped_event_base<const scope_type> *event, const gsml_operator condition_operator = gsml_operator::assignment)
		: condition<scope_type>(condition_operator)
	{
		this->event = event;
	}

	explicit event_condition(const std::string &value, const gsml_operator condition_operator)
		: event_condition(get_event(value), condition_operator)
	{
	}

	virtual const std::string &get_class_identifier() const override
	{
		static const std::string class_identifier = "event";
		return class_identifier;
	}

	virtual bool check_assignment(const scope_type *scope, const read_only_context &ctx) const override
	{
		Q_UNUSED(scope);
		Q_UNUSED(ctx);

		return game::get()->has_fired_event(this->event->get_event_pointer());
	}

	virtual std::string get_assignment_string(const size_t indent) const override
	{
		Q_UNUSED(indent);

		return std::format("The {} event has been triggered", this->event->get_name());
	}

private:
	const scoped_event_base<const scope_type> *event = nullptr;
};

}
