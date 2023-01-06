#include "metternich.h"

#include "game/event_option.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "database/gsml_property.h"
#include "script/context.h"
#include "script/effect/effect_list.h"

namespace metternich {

event_option::event_option() : name(event_option::default_name)
{
}

event_option::~event_option()
{
}

void event_option::process_gsml_property(const gsml_property &property)
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

void event_option::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "effects") {
		this->effects = std::make_unique<effect_list<const country>>();
		database::process_gsml_data(this->effects, scope);
	} else {
		throw std::runtime_error("Invalid event option scope: \"" + tag + "\".");
	}
}

void event_option::check() const
{
	if (this->effects != nullptr) {
		this->effects->check();
	}
}

std::string event_option::get_tooltip(const read_only_context &ctx) const
{
	if (!this->tooltip.empty()) {
		return this->tooltip;
	}

	if (this->effects != nullptr) {
		return this->get_effects_string(ctx);
	}

	return std::string();
}

std::string event_option::get_effects_string(const read_only_context &ctx) const
{
	if (this->effects != nullptr) {
		return this->effects->get_effects_string(ctx.current_country, ctx);
	}

	return std::string();
}

void event_option::do_effects(const country *country, context &ctx) const
{
	if (this->effects != nullptr) {
		this->effects->do_effects(country, ctx);
	}
}

}
