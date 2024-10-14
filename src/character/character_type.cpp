#include "metternich.h"

#include "character/character_type.h"

#include "character/advisor_category.h"
#include "character/character_attribute.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "script/modifier.h"
#include "ui/portrait.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"

namespace metternich {

character_type::character_type(const std::string &identifier)
	: named_data_entry(identifier), advisor_category(advisor_category::none), attribute(character_attribute::none), military_unit_category(military_unit_category::none)
{
}

character_type::~character_type()
{
}

void character_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "ruler_modifier") {
		this->ruler_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->ruler_modifier, scope);
	} else if (tag == "scaled_ruler_modifier") {
		this->scaled_ruler_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->scaled_ruler_modifier, scope);
	} else if (tag == "advisor_modifier") {
		this->advisor_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->advisor_modifier, scope);
	} else if (tag == "scaled_advisor_modifier") {
		this->scaled_advisor_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->scaled_advisor_modifier, scope);
	} else if (tag == "advisor_effects") {
		auto effect_list = std::make_unique<metternich::effect_list<const country>>();
		database::process_gsml_data(effect_list, scope);
		this->advisor_effects = std::move(effect_list);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character_type::check() const
{
	assert_throw(this->get_advisor_category() != advisor_category::none);

	if (this->get_attribute() == character_attribute::none) {
		throw std::runtime_error(std::format("Character type \"{}\" has no attribute.", this->get_identifier()));
	}

	if (this->get_advisor_effects() != nullptr) {
		this->get_advisor_effects()->check();
	}
}

}
