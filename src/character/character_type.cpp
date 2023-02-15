#include "metternich.h"

#include "character/character_type.h"

#include "character/attribute.h"
#include "script/condition/and_condition.h"
#include "ui/icon.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"

namespace metternich {

character_type::character_type(const std::string &identifier)
	: named_data_entry(identifier), primary_attribute(attribute::none), military_unit_category(military_unit_category::none)
{
}

void character_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditional_portraits") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const icon *portrait = icon::get(child_scope.get_tag());
			auto conditions = std::make_unique<and_condition<character>>();
			database::process_gsml_data(conditions, child_scope);
			this->conditional_portraits[portrait] = std::move(conditions);
		});
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->country_modifier, scope);
	} else if (tag == "province_modifier") {
		this->province_modifier = std::make_unique<modifier<const province>>();
		database::process_gsml_data(this->province_modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void character_type::check() const
{
	assert_throw(this->get_portrait() != nullptr);
	assert_throw(this->get_primary_attribute() != attribute::none);

	for (const auto &[portrait, conditions] : this->get_conditional_portraits()) {
		conditions->check_validity();
	}
}

QString character_type::get_primary_attribute_name_qstring() const
{
	return QString::fromStdString(get_attribute_name(this->get_primary_attribute()));
}

}
