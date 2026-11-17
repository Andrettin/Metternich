#include "metternich.h"

#include "script/effect/delayed_effect_instance.h"

#include "character/character.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "domain/domain.h"
#include "game/character_event.h"
#include "map/province.h"
#include "map/site.h"
#include "script/effect/scripted_effect.h"
#include "script/scope_traits.h"

namespace metternich {

template <typename scope_type>
delayed_effect_instance<scope_type>::delayed_effect_instance(const scripted_effect_base<scope_type> *scripted_effect, scope_type *scope, const metternich::context &ctx, const int turns)
	: delayed_effect_instance(scope, ctx, turns)
{
	this->scripted_effect = scripted_effect;
}

template <typename scope_type>
delayed_effect_instance<scope_type>::delayed_effect_instance(const scoped_event_base<scope_type> *event, scope_type *scope, const metternich::context &ctx, const int turns)
	: delayed_effect_instance(scope, ctx, turns)
{
	this->event = event;
}

template <typename scope_type>
delayed_effect_instance<scope_type>::delayed_effect_instance(scope_type *scope, const metternich::context &ctx, const int turns)
	: scope(scope), context(ctx), remaining_turns(turns)
{
}

template <typename scope_type>
void delayed_effect_instance<scope_type>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if constexpr (scope_has_events<scope_type>) {
		if (key == "event") {
			this->event = scope_traits_t<scope_type>::event_type::get(value);
			return;
		}
	}

	if (key == "scripted_effect") {
		this->scripted_effect = scope_traits_t<scope_type>::scripted_effect_type::get(value);
	} else if (key == "scope") {
		this->scope = scope_type::get(value);
	} else if (key == "remaining_turns") {
		this->remaining_turns = std::stoi(value);
	} else {
		throw std::runtime_error(std::format("Invalid delayed effect instance property: \"{}\".", key));
	}
}

template <typename scope_type>
void delayed_effect_instance<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "context") {
		this->context = metternich::context();
		scope.process(&this->context);
	} else {
		throw std::runtime_error("Invalid delayed effect instance scope: \"" + scope.get_tag() + "\".");
	}
}

template <typename scope_type>
gsml_data delayed_effect_instance<scope_type>::to_gsml_data() const
{
	gsml_data data;

	if (this->scripted_effect != nullptr) {
		data.add_property("scripted_effect", this->scripted_effect->get_identifier());
	} else {
		data.add_property("event", this->event->get_identifier());
	}

	data.add_property("scope", this->scope->get_identifier());

	data.add_property("remaining_turns", std::to_string(this->get_remaining_turns()));

	data.add_child(this->context.to_gsml_data("context"));

	return data;
}

template <typename scope_type>
QCoro::Task<void> delayed_effect_instance<scope_type>::do_effects()
{
	if (this->scripted_effect != nullptr) {
		co_await this->scripted_effect->get_effects().do_effects(scope, this->context);
	} else {
		metternich::context event_ctx(this->get_scope());
		event_ctx.source_scope = this->context.root_scope;

		co_await this->event->fire(this->get_scope(), event_ctx);
	}
}

template class delayed_effect_instance<const character>;
template class delayed_effect_instance<const domain>;
template class delayed_effect_instance<const province>;
template class delayed_effect_instance<const site>;

}
