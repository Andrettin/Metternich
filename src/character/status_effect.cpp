#include "metternich.h"

#include "character/status_effect.h"

#include "database/defines.h"
#include "script/effect/effect_list.h"
#include "util/random.h"
#include "util/string_conversion_util.h"

namespace metternich {

status_effect::status_effect(const std::string &identifier) : named_data_entry(identifier)
{
}

status_effect::~status_effect()
{
}

void status_effect::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "duration_per_caster_level") {
		this->duration_per_caster_level = string::to_duration(value);
	} else {
		data_entry::process_gsml_property(property);
	}
}

void status_effect::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "end_effects") {
		this->end_effects = std::make_unique<metternich::effect_list<const character>>();
		this->end_effects->process_gsml_data(scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void status_effect::check() const
{
	using namespace std::chrono_literals;

	if (this->get_adjective().empty()) {
		throw std::runtime_error(std::format("Status effect \"{}\" has no adjective.", this->get_identifier()));
	}

	if (this->get_saving_throw_type() == nullptr) {
		throw std::runtime_error(std::format("Status effect \"{}\" has no saving throw type.", this->get_identifier()));
	}

	if (this->get_duration_rounds().is_null() && this->get_duration_per_caster_level() == 0s) {
		throw std::runtime_error(std::format("Status effect \"{}\" has no duration.", this->get_identifier()));
	}
}

std::chrono::seconds status_effect::get_duration(const std::optional<int> &caster_level) const
{
	std::chrono::seconds duration(0);

	if (!this->get_duration_rounds().is_null()) {
		duration += random::get()->roll_dice(this->get_duration_rounds()) * defines::get()->get_combat_round_duration();
	}

	if (caster_level.has_value()) {
		duration += this->get_duration_per_caster_level() * caster_level.value();
	}

	return duration;
}

}
