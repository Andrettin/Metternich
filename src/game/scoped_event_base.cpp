#include "metternich.h"

#include "game/scoped_event_base.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/database.h"
#include "database/gsml_data.h"
#include "game/country_event.h"
#include "game/event_option.h"
#include "game/game.h"
#include "script/condition/and_condition.h"
#include "script/context.h"
#include "script/factor.h"

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
			scoped_event_base::trigger_random_events[this->get_trigger()].push_back(static_cast<event_type *>(this));
		} else {
			scoped_event_base::trigger_events[this->get_trigger()].push_back(static_cast<event_type *>(this));
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

template class scoped_event_base<country>;

}
