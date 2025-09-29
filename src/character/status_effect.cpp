#include "metternich.h"

#include "character/status_effect.h"

#include "script/effect/effect_list.h"

namespace metternich {

status_effect::status_effect(const std::string &identifier) : named_data_entry(identifier)
{
}

status_effect::~status_effect()
{
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
	if (this->get_saving_throw_type() == nullptr) {
		throw std::runtime_error(std::format("Status effect \"{}\" has no saving throw type.", this->get_identifier()));
	}

	if (this->get_duration_rounds_dice().is_null()) {
		throw std::runtime_error(std::format("Status effect \"{}\" has no duration rounds dice.", this->get_identifier()));
	}
}

}
