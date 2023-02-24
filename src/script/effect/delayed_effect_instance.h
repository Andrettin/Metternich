#pragma once

#include "script/context.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

template <typename scope_type>
class scoped_event_base;

template <typename scope_type>
class scripted_effect_base;

template <typename scope_type>
class delayed_effect_instance final
{
public:
	delayed_effect_instance()
	{
	}

	explicit delayed_effect_instance(const scripted_effect_base<scope_type> *scripted_effect, scope_type *scope, const context &ctx, const int turns);
	explicit delayed_effect_instance(const scoped_event_base<scope_type> *event, scope_type *scope, const context &ctx, const int cyclturnses);

private:
	explicit delayed_effect_instance(scope_type *scope, const context &ctx, const int turns);

public:
	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	scope_type *get_scope() const
	{
		return this->scope;
	}

	int get_remaining_turns() const
	{
		return this->remaining_turns;
	}

	void decrement_remaining_turns()
	{
		--this->remaining_turns;
	}

	void do_effects();

private:
	const scripted_effect_base<scope_type> *scripted_effect = nullptr;
	const scoped_event_base<scope_type> *event = nullptr;
	scope_type *scope = nullptr;
	metternich::context context;
	int remaining_turns = 0;
};

extern template class delayed_effect_instance<const character>;
extern template class delayed_effect_instance<const country>;
extern template class delayed_effect_instance<const province>;

}
