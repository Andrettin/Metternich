#include "metternich.h"

#include "character/advisor_type.h"

#include "character/advisor_category.h"
#include "script/condition/and_condition.h"
#include "ui/icon.h"
#include "util/assert_util.h"

namespace metternich {

advisor_type::advisor_type(const std::string &identifier)
	: named_data_entry(identifier), category(advisor_category::none)
{
}

void advisor_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditional_portraits") {
		scope.for_each_child([&](const gsml_data &child_scope) {
			const icon *portrait = icon::get(child_scope.get_tag());
			auto conditions = std::make_unique<and_condition<character>>();
			database::process_gsml_data(conditions, child_scope);
			this->conditional_portraits[portrait] = std::move(conditions);
		});
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(this->modifier, scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void advisor_type::check() const
{
	assert_throw(this->get_category() != advisor_category::none);
	assert_throw(this->get_portrait() != nullptr);

	for (const auto &[portrait, conditions] : this->get_conditional_portraits()) {
		conditions->check_validity();
	}
}

}
