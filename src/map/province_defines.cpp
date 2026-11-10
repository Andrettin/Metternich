#include "metternich.h"

#include "map/province_defines.h"

#include "database/gsml_data.h"
#include "economy/commodity.h"
#include "economy/province_taxation_type.h"
#include "population/population_defines.h"
#include "util/assert_util.h"
#include "util/centesimal_int.h"
#include "util/string_conversion_util.h"

namespace metternich {

province_defines::province_defines()
{
}

province_defines::~province_defines()
{
}

void province_defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "province_level_commodity_costs_per_level") {
		scope.to_map(this->province_level_commodity_costs_per_level);
	} else if (tag == "province_taxation_per_level") {
		scope.for_each_child([this](const gsml_data &child_scope) {
			const province_taxation_type taxation_type = string::to_enum<province_taxation_type>(child_scope.get_tag());

			child_scope.for_each_property([this, taxation_type](const gsml_property &property) {
				const int level = std::stoi(property.get_key());
				dice dice(property.get_value());
				if (dice.get_modifier() < 0) {
					dice.set_min_value(0);
				}

				this->province_taxation_per_level[taxation_type][level] = std::move(dice);
			});
		});
	} else {
		defines_base::process_gsml_scope(scope);
	}
}

void province_defines::check() const
{
	if (this->get_default_province_terrain() == nullptr) {
		throw std::runtime_error("No default province terrain is defined.");
	}

	if (this->get_default_water_zone_terrain() == nullptr) {
		throw std::runtime_error("No default water zone terrain is defined.");
	}
}

int province_defines::get_province_population_capacity_level_per_level() const
{
	return this->province_population_capacity_level_per_level;
}

int province_defines::get_province_level_for_population(const int64_t population) const
{
	const centesimal_int population_capacity_level = centesimal_int(population) / population_defines::get()->get_base_population_size();

	return (population_capacity_level / this->get_province_population_capacity_level_per_level()).to_rounded_int();
}

const dice &province_defines::get_province_taxation_for_level(const province_taxation_type taxation_type, const int level) const
{
	static constexpr dice null_dice;

	if (taxation_type == province_taxation_type::no_taxation) {
		return null_dice;
	}

	const auto find_iterator = this->province_taxation_per_level.find(taxation_type);
	assert_throw(find_iterator != this->province_taxation_per_level.end());

	const auto sub_find_iterator = find_iterator->second.find(level);
	if (sub_find_iterator != find_iterator->second.end()) {
		return sub_find_iterator->second;
	}

	//if the level is higher than the highest provided level for taxation, use that for the largest level
	const auto last_iterator = find_iterator->second.rbegin();
	if (level > last_iterator->first) {
		return last_iterator->second;
	}

	assert_throw(false);
	return null_dice;
}

}

#include "moc_province_defines.cpp"
