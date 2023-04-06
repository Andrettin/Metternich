#include "metternich.h"

#include "spell/spell.h"

#include "spell/spell_effect.h"
#include "spell/spell_target.h"
#include "unit/military_unit_category.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

spell::spell(const std::string &identifier)
	: named_data_entry(identifier), target(spell_target::none)
{
}

spell::~spell()
{
}

void spell::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "military_unit_categories") {
		for (const std::string &value : values) {
			this->military_unit_categories.push_back(enum_converter<military_unit_category>::to_enum(value));
		}
	} else if (tag == "effects") {
		scope.for_each_element([&](const gsml_property &property) {
			auto effect = spell_effect::from_gsml_property(property);
			this->effects.push_back(std::move(effect));
		}, [&](const gsml_data &child_scope) {
			auto effect = spell_effect::from_gsml_scope(child_scope);
			this->effects.push_back(std::move(effect));
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void spell::check() const
{
	assert_throw(this->get_target() != spell_target::none);
	assert_throw(this->get_icon() != nullptr);
	assert_throw(this->get_mana_cost() > 0);
	//assert_throw(!this->get_military_unit_categories().empty());
	assert_throw(!this->effects.empty());
}

bool spell::is_available_for_military_unit_category(const military_unit_category military_unit_category) const
{
	return vector::contains(this->get_military_unit_categories(), military_unit_category);
}

}
