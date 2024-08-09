#include "metternich.h"

#include "game/scoped_event_base.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/database.h"
#include "database/gsml_data.h"
#include "game/event_option.h"
#include "game/event_random_group.h"
#include "game/event_trigger.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/effect/delayed_effect_instance.h"
#include "script/factor.h"
#include "script/mean_time_to_happen.h"
#include "util/assert_util.h"
#include "util/fractional_int.h"
#include "util/random.h"
#include "util/vector_random_util.h"

namespace metternich {

template <typename scope_type>
const scope_type *scoped_event_base<scope_type>::get_scope_from_context(const read_only_context &ctx)
{
	if constexpr (std::is_same_v<scope_type, const character>) {
		return std::get<const character *>(ctx.root_scope);
	} else if constexpr (std::is_same_v<scope_type, const country>) {
		return std::get<const country *>(ctx.root_scope);
	} else if constexpr (std::is_same_v<scope_type, const province>) {
		return std::get<const province *>(ctx.root_scope);
	}
}

template <typename scope_type>
bool scoped_event_base<scope_type>::is_player_scope(const scope_type *scope)
{
	if constexpr (std::is_same_v<scope_type, const character>) {
		return false;
	} else if constexpr (std::is_same_v<scope_type, const country>) {
		return scope == game::get()->get_player_country();
	} else if constexpr (std::is_same_v<scope_type, const province>) {
		return scope->get_game_data()->get_owner() == game::get()->get_player_country();
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::check_events_for_scope(const scope_type *scope, const event_trigger trigger, const context &ctx)
{
	assert_throw(trigger != event_trigger::none);

	for (const scoped_event_base *event : scoped_event_base::get_trigger_events(trigger)) {
		if (!event->can_fire(scope, ctx)) {
			continue;
		}

		event->fire(scope, ctx);
	}

	scoped_event_base::check_random_events_for_scope(scope, ctx, scoped_event_base::get_trigger_random_events(trigger), 0);
	scoped_event_base::check_random_event_groups_for_scope(scope, trigger, ctx);

	if (trigger == event_trigger::quarterly_pulse) {
		scoped_event_base::check_mtth_events_for_scope(scope);
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::check_events_for_scope(const scope_type *scope, const event_trigger trigger)
{
	scoped_event_base<scope_type>::check_events_for_scope(scope, trigger, context(scope));
}

template <typename scope_type>
void scoped_event_base<scope_type>::check_random_events_for_scope(const scope_type *scope, const context &ctx, const std::vector<const scoped_event_base *> &potential_events, const int delay)
{
	std::vector<const scoped_event_base *> random_events;

	for (const scoped_event_base *event : potential_events) {
		if (event == nullptr) {
			random_events.push_back(event);
			continue;
		}

		const int weight = event->get_random_weight_factor()->calculate(scope).to_int();

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

		if (event->can_fire(scope, ctx)) {
			if (delay > 0) {
				auto delayed_effect = std::make_unique<delayed_effect_instance<scope_type>>(event, scope, ctx, delay);
				game::get()->add_delayed_effect(std::move(delayed_effect));
			} else {
				event->fire(scope, ctx);
			}
			break;
		}

		std::erase(random_events, event);
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::check_random_event_groups_for_scope(const scope_type *scope, const event_trigger trigger, const context &ctx)
{
	for (const event_random_group *random_group : event_random_group::get_all_of_trigger(trigger)) {
		const std::vector<const scoped_event_base *> &potential_events = random_group->get_events<scope_type>();
		if (potential_events.empty()) {
			continue;
		}

		scoped_event_base::check_random_events_for_scope(scope, ctx, potential_events, random_group->get_delay(game::get()->get_year()));
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::check_mtth_events_for_scope(const scope_type *scope)
{
	const read_only_context ctx(scope);

	for (const scoped_event_base *event : scoped_event_base::mtth_events) {
		if (!event->can_fire(scope, ctx)) {
			continue;
		}

		const centesimal_int mtth = event->get_mean_time_to_happen()->calculate(scope, game::get()->get_year());
		bool should_fire = false;

		if (mtth <= 1) {
			should_fire = true;
		} else {
			const int fire_chance = (decimillesimal_int(1) / mtth).get_value();
			assert_throw(fire_chance > 0);
			should_fire = random::get()->generate(10000) < fire_chance;
		}

		if (should_fire) {
			event->fire(scope, context(scope));
		}
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
		this->random_weight_factor = std::make_unique<factor<std::remove_const_t<scope_type>>>();
		database::process_gsml_data(this->random_weight_factor, scope);
		return true;
	} else if (tag == "mean_time_to_happen") {
		this->mean_time_to_happen = std::make_unique<metternich::mean_time_to_happen<std::remove_const_t<scope_type>>>();
		database::process_gsml_data(this->mean_time_to_happen, scope);
		return true;
	} else if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<std::remove_const_t<scope_type>>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
		return true;
	} else if (tag == "immediate_effects") {
		this->immediate_effects = std::make_unique<effect_list<scope_type>>();
		database::process_gsml_data(this->immediate_effects, scope);
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
	if (this->get_random_group() != nullptr) {
		this->get_random_group()->add_event(this);
	} else if (this->get_trigger() != event_trigger::none) {
		if (this->is_random()) {
			scoped_event_base::trigger_random_events[this->get_trigger()].push_back(this);
		} else {
			scoped_event_base::trigger_events[this->get_trigger()].push_back(this);
		}
	} else if (this->get_mean_time_to_happen() != nullptr) {
		scoped_event_base::mtth_events.push_back(this);
	}

	if (this->get_option_count() == 0) {
		//add an empty option
		this->options.push_back(std::make_unique<event_option<scope_type>>());
	}
}

template <typename scope_type>
void scoped_event_base<scope_type>::check() const
{
	if (this->get_random_weight_factor() != nullptr) {
		this->get_random_weight_factor()->check();
	}

	if (this->get_mean_time_to_happen() != nullptr) {
		this->get_mean_time_to_happen()->check();

		if (this->get_trigger() != event_trigger::none) {
			throw std::runtime_error("Event \"" + this->get_identifier() + "\" has both a mean time to happen and a trigger.");
		}

		if (this->get_random_group() != nullptr) {
			throw std::runtime_error("Event \"" + this->get_identifier() + "\" has both a mean time to happen and a random group.");
		}
	}

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
		this->random_weight_factor = std::make_unique<factor<std::remove_const_t<scope_type>>>(weight);
	} else {
		this->random_weight_factor.reset();
	}
}

template <typename scope_type>
bool scoped_event_base<scope_type>::can_fire(const scope_type *scope, const read_only_context &ctx) const
{
	if (this->fires_only_once() && this->has_fired()) {
		return false;
	}

	if (this->get_conditions() != nullptr && !this->get_conditions()->check(scope, ctx)) {
		return false;
	}

	return true;
}

template <typename scope_type>
void scoped_event_base<scope_type>::do_immediate_effects(scope_type *scope, context &ctx) const
{
	if (this->immediate_effects != nullptr) {
		this->immediate_effects->do_effects(scope, ctx);
	}
}

template <typename scope_type>
bool scoped_event_base<scope_type>::is_option_available(const int option_index, const read_only_context &ctx) const
{
	const condition<std::remove_const_t<scope_type>> *conditions = this->get_options().at(option_index)->get_conditions();

	if (conditions == nullptr) {
		return true;
	}

	const scope_type *scope = scoped_event_base::get_scope_from_context(ctx);

	return conditions->check(scope, ctx);
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
	const scope_type *scope = scoped_event_base::get_scope_from_context(ctx);
	this->get_options().at(option_index)->do_effects(scope, ctx);
}

template <typename scope_type>
void scoped_event_base<scope_type>::fire(const scope_type *scope, const context &ctx) const
{
	if (this->fires_only_once()) {
		assert_throw(!this->has_fired());
	}

	scoped_event_base::fired_events.insert(this);

	context event_ctx = ctx;

	this->do_immediate_effects(scope, event_ctx);

	if (scoped_event_base::is_player_scope(scope)) {
		this->create_instance(event_ctx);
	} else {
		//the event doesn't need to be displayed for AIs; instead, it is processed immediately
		//pick a random option out of the available ones
		std::vector<const event_option<scope_type> *> options;

		for (const std::unique_ptr<event_option<scope_type>> &option : this->get_options()) {
			if (option->get_conditions() != nullptr && !option->get_conditions()->check(scope, event_ctx)) {
				continue;
			}

			options.push_back(option.get());
		}

		if (!options.empty()) {
			const event_option<scope_type> *option = vector::get_random(options);
			option->do_effects(scope, event_ctx);
		}
	}
}

template class scoped_event_base<const character>;
template class scoped_event_base<const country>;
template class scoped_event_base<const province>;

}
