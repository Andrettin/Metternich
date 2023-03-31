#include "metternich.h"

#include "game/event_option.h"

#include "character/character.h"
#include "character/trait.h"
#include "database/database.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "game/event.h"
#include "script/context.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

template <typename scope_type>
event_option<scope_type>::event_option()
	: name(event::option_default_name)
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
	} else if (key == "tooltip_info_trait") {
		this->tooltip_info_trait = trait::get(value);
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

	if (tag == "conditions") {
		this->conditions = std::make_unique<and_condition<std::remove_const_t<scope_type>>>();
		database::process_gsml_data(this->conditions, scope);
	} else if (tag == "effects") {
		this->effects = std::make_unique<effect_list<scope_type>>();
		database::process_gsml_data(this->effects, scope);
	} else {
		throw std::runtime_error("Invalid event option scope: \"" + tag + "\".");
	}
}

template <typename scope_type>
void event_option<scope_type>::check() const
{
	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->effects != nullptr) {
		this->effects->check();
	}
}

template <typename scope_type>
std::string event_option<scope_type>::get_tooltip(const read_only_context &ctx) const
{
	std::string str;

	if (!this->tooltip.empty()) {
		str += this->tooltip;
	} else {
		str += this->get_effects_string(ctx);
	}

	if (this->tooltip_info_trait != nullptr) {
		str = "(This option is available due to the " + this->tooltip_info_trait->get_name() + " trait)\n" + str;
	}

	return str;
}

template <typename scope_type>
std::string event_option<scope_type>::get_effects_string(const read_only_context &ctx) const
{
	if (this->effects != nullptr) {
		const scope_type *scope = nullptr;

		if constexpr (std::is_same_v<scope_type, const character>) {
			scope = std::get<const character *>(ctx.root_scope);
		} else if constexpr (std::is_same_v<scope_type, const country>) {
			scope = std::get<const country *>(ctx.root_scope);
		} else if constexpr (std::is_same_v<scope_type, const province>) {
			scope = std::get<const province *>(ctx.root_scope);
		} else {
			assert_throw(false);
		}

		assert_throw(scope != nullptr);

		std::string str = this->effects->get_effects_string(scope, ctx);

		if (str.empty()) {
			return effect_list<scope_type>::no_effect_string;
		}

		return str;
	}

	return effect_list<scope_type>::no_effect_string;
}

template <typename scope_type>
void event_option<scope_type>::do_effects(scope_type *scope, context &ctx) const
{
	if (this->effects != nullptr) {
		this->effects->do_effects(scope, ctx);
	}
}

template class event_option<const character>;
template class event_option<const country>;
template class event_option<const province>;

}
