#include "metternich.h"

#include "map/site_history.h"

#include "database/gsml_data.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_slot_type.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/improvement_slot.h"
#include "infrastructure/wonder.h"
#include "map/site.h"
#include "util/assert_util.h"

namespace metternich {

void site_history::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "improvements") {
		const improvement *improvement = improvement::get(value);
		const improvement_slot slot = improvement->get_slot();

		switch (property.get_operator()) {
			case gsml_operator::addition:
				this->improvements[slot] = improvement;
				break;
			case gsml_operator::subtraction:
				if (this->get_improvement(slot) != improvement) {
					throw std::runtime_error(std::format("Tried to remove the \"{}\" improvement in the history of the \"{}\" site, but the improvement was not present.", improvement->get_identifier(), this->site->get_identifier()));
				}

				this->improvements.erase(slot);
				break;
			default:
				assert_throw(false);
		}
	} else if (key == "buildings") {
		const building_type *building = building_type::get(value);
		const building_slot_type *slot_type = building->get_slot_type();

		switch (property.get_operator()) {
			case gsml_operator::addition:
				this->buildings[slot_type] = building;
				break;
			case gsml_operator::subtraction:
				if (this->get_building(slot_type) != building) {
					throw std::runtime_error(std::format("Tried to remove the \"{}\" building in the history of the \"{}\" site, but the building was not present.", building->get_identifier(), this->site->get_identifier()));
				}

				this->buildings.erase(slot_type);
				break;
			default:
				assert_throw(false);
		}
	} else if (key == "wonders") {
		const wonder *wonder = wonder::get(value);
		const building_slot_type *slot_type = wonder->get_building()->get_slot_type();

		switch (property.get_operator()) {
			case gsml_operator::addition:
				this->wonders[slot_type] = wonder;
				break;
			case gsml_operator::subtraction:
				if (this->get_wonder(slot_type) != wonder) {
					throw std::runtime_error(std::format("Tried to remove the \"{}\" wonder in the history of the \"{}\" site, but the wonder was not present.", wonder->get_identifier(), this->site->get_identifier()));
				}

				this->wonders.erase(slot_type);
				break;
			default:
				assert_throw(false);
		}
	} else {
		data_entry_history::process_gsml_property(property);
	}
}

void site_history::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "improvements") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const improvement_slot slot = enum_converter<improvement_slot>::to_enum(key);
			const improvement *improvement = improvement::get(value);

			if (improvement == nullptr) {
				this->improvements.erase(slot);
			} else {
				this->improvements[slot] = improvement;
			}
		});
	} else if (tag == "buildings") {
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
	} else if (tag == "wonders") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const building_slot_type *slot_type = building_slot_type::get(key);
			const wonder *wonder = wonder::get(value);

			if (wonder == nullptr) {
				this->wonders.erase(slot_type);
			} else {
				this->wonders[slot_type] = wonder;
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
