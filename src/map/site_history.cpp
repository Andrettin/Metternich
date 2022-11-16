#include "metternich.h"

#include "map/site_history.h"

#include "database/gsml_data.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"

namespace metternich {

void site_history::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "buildings") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const building_slot_type *slot_type = building_slot_type::get(key);
			const building_type *building = building_type::get(value);

			if (building == nullptr) {
				this->buildings.erase(slot_type);
			} else {
				this->buildings[slot_type] = building;
			}
		});
	} else if (tag == "population_groups") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key_str = property.get_key();
			const population_group_key key(key_str);

			const std::string &value = property.get_value();
			const int population = std::stoi(value);

			if (population == 0) {
				this->population_groups.erase(key);
			} else {
				this->population_groups[key] = population;
			}
		});
	} else {
		data_entry_history::process_gsml_scope(scope);
	}
}

}
