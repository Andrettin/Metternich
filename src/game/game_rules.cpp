#include "metternich.h"

#include "game/game_rules.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "game/game_rule.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/map_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

game_rules::game_rules()
{
	for (const game_rule *rule : game_rule::get_all()) {
		this->values[rule] = rule->get_default_value();
	}
}

void game_rules::process_gsml_property(const gsml_property &property)
{
	const game_rule *rule = game_rule::try_get(property.get_key());
	if (rule != nullptr) {
		assert_throw(property.get_operator() == gsml_operator::assignment);
		this->values[rule] = string::to_bool(property.get_value());
	} else {
		database::get()->process_gsml_property_for_object(this, property);
	}
}

void game_rules::process_gsml_scope(const gsml_data &scope)
{
	database::get()->process_gsml_scope_for_object(this, scope);
}

gsml_data game_rules::to_gsml_data() const
{
	gsml_data data;

	for (const auto &[rule, value] : this->get_values()) {
		data.add_property(rule->get_identifier(), string::from_bool(value));
	}

	return data;
}

QVariantList game_rules::get_rules_qvariant_list() const
{
	return container::to_qvariant_list(archimedes::map::get_keys(this->get_values()));

}

QVariantList game_rules::get_values_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_values());
}

}
