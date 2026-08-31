#include "metternich.h"

#include "infrastructure/holding_defines.h"

#include "economy/commodity.h"
#include "infrastructure/construction_type.h"
#include "util/assert_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

holding_defines::holding_defines()
{
}

holding_defines::~holding_defines()
{
}

const std::set<std::string> &holding_defines::get_database_dependencies() const
{
	static const std::set<std::string> database_dependencies = {
		commodity::class_identifier //needed for the suffixes for holding commodity costs
	};
	return database_dependencies;
}

void holding_defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "consumption_per_holding_level") {
		scope.for_each_property([this](const gsml_property &property) {
			const int holding_level = std::stoi(property.get_key());
			const centesimal_int consumption(property.get_value());

			this->consumption_per_holding_level[holding_level] = consumption;
		});
	} else if (tag == "construction_level_commodity_costs") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const construction_type construction_type = magic_enum::enum_cast<metternich::construction_type>(child_tag).value();

			child_scope.for_each_property([this, construction_type](const gsml_property &property) {
				const commodity *commodity = commodity::get(property.get_key());
				this->construction_level_commodity_costs[construction_type][commodity] = commodity->string_to_value(property.get_value());
			});
		});
	} else if (tag == "construction_level_commodity_costs_per_level") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const std::string &child_tag = child_scope.get_tag();
			const construction_type construction_type = magic_enum::enum_cast<metternich::construction_type>(child_tag).value();

			child_scope.for_each_property([this, construction_type](const gsml_property &property) {
				const commodity *commodity = commodity::get(property.get_key());
				this->construction_level_commodity_costs_per_level[construction_type][commodity] = commodity->string_to_value(property.get_value());
			});
		});
	} else {
		defines_base::process_gsml_scope(scope);
	}
}

const centesimal_int &holding_defines::get_consumption_for_holding_level(const int holding_level) const
{
	const auto find_iterator = this->consumption_per_holding_level.upper_bound(holding_level);
	assert_throw(find_iterator != this->consumption_per_holding_level.begin());
	return std::prev(find_iterator)->second;
}

}
