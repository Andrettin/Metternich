#include "metternich.h"

#include "game/scoped_event_base.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/database.h"
#include "database/gsml_data.h"
#include "game/event_option.h"
#include "game/game.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/factor.h"
#include "util/assert_util.h"
#include "util/vector_random_util.h"

namespace metternich {

template <typename scope_type>
bool scoped_event_base<scope_type>::is_player_scope(const scope_type *scope)
{
	if constexpr (std::is_same_v<scope_type, character>) {
		return scope == game::get()->get_player_country()->get_game_data()->get_ruler();
	} else if constexpr (std::is_same_v<scope_type, country>) {
		return scope == game::get()->get_player_country();
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::check_events_for_scope(const scope_type *scope, const event_trigger trigger)
{
	assert_throw(trigger != event_trigger::none);

	const read_only_context ctx = read_only_context::from_scope(scope);

	for (const scoped_event_base *event : scoped_event_base::get_trigger_events(trigger)) {
		if (event->get_conditions() != nullptr && !event->get_conditions()->check(scope, ctx)) {
			continue;
		}

		event->fire(scope, context::from_scope(scope));
	}

	scoped_event_base::check_random_events_for_scope(scope, trigger, ctx);
}

template <typename scope_type>
void scoped_event_base<scope_type>::check_random_events_for_scope(const scope_type *scope, const event_trigger trigger, const read_only_context &ctx)
{
	std::vector<const scoped_event_base *> random_events;

	for (const scoped_event_base *event : scoped_event_base::get_trigger_random_events(trigger)) {
		if (event == nullptr) {
			random_events.push_back(event);
			continue;
		}

		const int weight = event->get_random_weight_factor()->calculate(scope);

		for (int i = 0; i < weight; ++i) {
			random_events.push_back(event);
		}
	}

	while (!random_events.empty()) {
		const scoped_event_base *event = vector::get_random(random_events);

		if (event == nullptr) {
			//a null event represents no event happening for the player for this check
			break;
		}

		if (event->get_conditions() == nullptr || event->get_conditions()->check(scope, ctx)) {
			event->fire(scope, context::from_scope(scope));
			break;
		}

		std::erase(random_events, event);
	}
}

template <typename scope_type>
scoped_event_base<scope_type>::scoped_event_base()
{
}

template <typename scope_type>
scoped_event_base<scope_type>::~scoped_event_base()
{
}

template <typename scope_type>
bool scoped_event_base<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "random_weight_factor") {
		this->random_weight_factor = std::make_unique<factor<scope_type>>();
		database::process_gsml_data(this->random_weight_factor, scope);
		return true;
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<scope_type>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
		return true;
	} else if (tag == "option") {
		auto option = std::make_unique<event_option<scope_type>>();
		database::process_gsml_data(option, scope);
		this->options.push_back(std::move(option));
		return true;
	} else {
		return false;
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::initialize()
{
	if (this->get_trigger() != event_trigger::none) {
		if (this->is_random()) {
			scoped_event_base::trigger_random_events[this->get_trigger()].push_back(this);
		} else {
			scoped_event_base::trigger_events[this->get_trigger()].push_back(this);
		}
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::check() const
{
	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	for (const std::unique_ptr<event_option<scope_type>> &option : this->get_options()) {
		option->check();
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::set_random_weight(const int weight)
{
	if (weight != 0) {
		this->random_weight_factor = std::make_unique<factor<scope_type>>(weight);
	} else {
		this->random_weight_factor.reset();
	}
}

template <typename scope_type>
const std::string &scoped_event_base<scope_type>::get_option_name(const int option_index) const
{
	return this->get_options().at(option_index)->get_name();
}

template <typename scope_type>
std::string scoped_event_base<scope_type>::get_option_tooltip(const int option_index, const read_only_context &ctx) const
{
	return this->get_options().at(option_index)->get_tooltip(ctx);
}

template <typename scope_type>
void scoped_event_base<scope_type>::do_option_effects(const int option_index, context &ctx) const
{
	const scope_type *scope = nullptr;

	if constexpr (std::is_same_v<scope_type, character>) {
		scope = ctx.current_character;
	} else if constexpr (std::is_same_v<scope_type, country>) {
		scope = ctx.current_country;
	}

	this->get_options().at(option_index)->do_effects(scope, ctx);
}

template <typename scope_type>
void scoped_event_base<scope_type>::fire(const scope_type *scope, const context &ctx) const
{
	if (scoped_event_base::is_player_scope(scope)) {
		this->create_instance(ctx);
	} else {
		//the event doesn't need to be displayed for AIs; instead, it should be processed immediately
		return;
	}
}

template class scoped_event_base<character>;
template class scoped_event_base<country>;

}
