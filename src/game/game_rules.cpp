#include "metternich.h"

#include "game/game_rules.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "game/game_rule.h"
#include "game/game_rule_group.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/exception_util.h"
#include "util/map_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

game_rules::game_rules()
{
	for (const game_rule *rule : game_rule::get_all()) {
		this->values[rule] = false;
		this->set_value(rule, rule->get_default_value());
	}
}

void game_rules::process_gsml_property(const gsml_property &property)
{
	try {
		const game_rule *rule = game_rule::try_get(property.get_key());
		if (rule != nullptr) {
			assert_throw(property.get_operator() == gsml_operator::assignment);
			//for hidden rules, don't change their value from the default one
			if (!rule->is_hidden()) { 
				this->values[rule] = string::to_bool(property.get_value());
			}
		} else {
			//this can throw an exception if an old rule was removed
			database::get()->process_gsml_property_for_object(this, property);
		}
	} catch (...) {
		exception::report(std::current_exception());
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
		if (rule->is_hidden()) {
			continue;
		}

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

bool game_rules::get_value(const archimedes::game_rule *rule) const
{
	const auto find_iterator = this->values.find(rule);
	if (find_iterator != this->values.end()) {
		return find_iterator->second;
	}
	return false;
}

bool game_rules::get_value(const std::string &rule_identifier) const
{
	try {
		return this->get_value(game_rule::get(rule_identifier));
	} catch (...) {
		exception::report(std::current_exception());
		return false;
	}
}

void game_rules::set_value(const game_rule *rule, const bool value)
{
	assert_throw(rule != nullptr);
	assert_throw(!value || this->is_rule_available(rule));

	if (value == this->get_value(rule)) {
		return;
	}

	this->values[rule] = value;

	if (value) {
		if (rule->get_group() != nullptr) {
			for (const game_rule *group_rule : rule->get_group()->get_rules()) {
				if (group_rule != rule && this->get_value(group_rule)) {
					this->set_value(group_rule, false);
				}
			}
		}
	} else {
		for (const game_rule *requiring_rule : rule->get_requiring_rules()) {
			this->set_value(requiring_rule, false);
		}
	}

	emit values_changed();
}

bool game_rules::is_rule_available(const game_rule *rule) const
{
	for (const game_rule *required_rule : rule->get_required_rules()) {
		if (!this->get_value(required_rule)) {
			return false;
		}
	}

	return true;
}

}
