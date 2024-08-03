#include "metternich.h"

#include "character/advisor_type.h"

#include "character/advisor_category.h"
#include "script/condition/and_condition.h"
#include "script/effect/effect_list.h"
#include "ui/portrait.h"
#include "util/assert_util.h"

namespace metternich {

advisor_type::advisor_type(const std::string &identifier)
	: named_data_entry(identifier), category(advisor_category::none)
{
}

advisor_type::~advisor_type()
{
}

void advisor_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(this->modifier, scope);
	} else if (tag == "scaled_modifier") {
		this->scaled_modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(this->scaled_modifier, scope);
	} else if (tag == "effects") {
		auto effect_list = std::make_unique<metternich::effect_list<const country>>();
		database::process_gsml_data(effect_list, scope);
		this->effects = std::move(effect_list);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void advisor_type::check() const
{
	assert_throw(this->get_category() != advisor_category::none);
}

}
