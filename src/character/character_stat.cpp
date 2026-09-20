#include "metternich.h"

#include "character/character_stat.h"

#include "database/gsml_data.h"
#include "script/modifier.h"
#include "script/modifier_effect/challenge_rating_modifier_effect.h"
#include "util/assert_util.h"
#include "util/string_util.h"

namespace metternich {

const std::unique_ptr<const character_stat> character_stat::armor_class = []() {
	auto armor_class = std::make_unique<character_stat>("armor_class");
	armor_class->set_name("Armor Class");
	armor_class->affect_military_unit_stats = true;

	//an armor class bonus of +10 increases challenge rating by 1
	auto value_modifier_10 = std::make_unique<metternich::modifier<const character>>();
	value_modifier_10->add_modifier_effect(std::make_unique<challenge_rating_modifier_effect>(decimillesimal_int(1)));
	armor_class->value_modifiers[centesimal_int(10)] = std::move(value_modifier_10);

	return armor_class;
}();

character_stat::character_stat(const std::string &identifier) : named_data_entry(identifier)
{
	const auto result = character_stat::stats_by_identifier.insert_or_assign(this->get_identifier(), this);
	if (!result.second) {
		throw std::runtime_error(std::format("Character stat with identifier \"{}\" already exists.", this->get_identifier()));
	}
}

character_stat::~character_stat()
{
}

void character_stat::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "value_modifiers") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const centesimal_int value(child_tag);
			if (!this->value_modifiers.contains(value)) {
				this->value_modifiers[value] = std::make_unique<metternich::modifier<const character>>();
			}
			this->value_modifiers[value]->process_gsml_data(child_scope);

			if (value.get_fractional_value() != 0) {
				this->exceptional_values.insert(value.to_int());
			}
		});
	} else if (tag == "recurring_value_modifiers") {
		static constexpr int max_value = std::numeric_limits<uint8_t>::max();

		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const int value_interval = std::stoi(child_tag);
			for (int i = value_interval; i <= max_value; i += value_interval) {
				const centesimal_int value(i);
				if (!this->value_modifiers.contains(value)) {
					this->value_modifiers[value] = std::make_unique<metternich::modifier<const character>>();
				}
				this->value_modifiers[value]->process_gsml_data(child_scope);
			}
		});
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

bool character_stat::affects_military_unit_stats() const
{
	return this->affect_military_unit_stats;
}

}
