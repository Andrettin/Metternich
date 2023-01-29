#include "metternich.h"

#include "game/event_option.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "game/event.h"
#include "script/context.h"
#include "script/effect/effect_list.h"
#include "util/assert_util.h"

namespace metternich {

template <typename scope_type>
event_option<scope_type>::event_option() : name(event::option_default_name)
{
}

template <typename scope_type>
event_option<scope_type>::~event_option()
{
}

template <typename scope_type>
void event_option<scope_type>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "name") {
		this->name = value;
	} else if (key == "tooltip") {
		this->tooltip = value;
	} else if (key == "ai_weight") {
		this->ai_weight = std::stoi(value);
	} else {
		throw std::runtime_error("Invalid event option property: \"" + key + "\".");
	}
}

template <typename scope_type>
void event_option<scope_type>::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "effects") {
		this->effects = std::make_unique<effect_list<const scope_type>>();
		database::process_gsml_data(this->effects, scope);
	} else {
		throw std::runtime_error("Invalid event option scope: \"" + tag + "\".");
	}
}

template <typename scope_type>
void event_option<scope_type>::check() const
{
	if (this->effects != nullptr) {
		this->effects->check();
	}
}

template <typename scope_type>
std::string event_option<scope_type>::get_tooltip(const read_only_context &ctx) const
{
	if (!this->tooltip.empty()) {
		return this->tooltip;
	}

	if (this->effects != nullptr) {
		return this->get_effects_string(ctx);
	}

	return std::string();
}

template <typename scope_type>
std::string event_option<scope_type>::get_effects_string(const read_only_context &ctx) const
{
	if (this->effects != nullptr) {
		const scope_type *scope = nullptr;

		if constexpr (std::is_same_v<scope_type, character>) {
			scope = ctx.current_character;
		} else if constexpr (std::is_same_v<scope_type, country>) {
			scope = ctx.current_country;
		}

		assert_throw(scope != nullptr);

		return this->effects->get_effects_string(scope, ctx);
	}

	return std::string();
}

template <typename scope_type>
void event_option<scope_type>::do_effects(const scope_type *scope, context &ctx) const
{
	if (this->effects != nullptr) {
		this->effects->do_effects(scope, ctx);
	}
}

template class event_option<country>;

}
