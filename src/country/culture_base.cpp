#include "metternich.h"

#include "country/culture_base.h"

#include "country/cultural_group.h"
#include "country/culture_history.h"
#include "country/government_type.h"
#include "infrastructure/building_class.h"
#include "infrastructure/building_type.h"
#include "population/population_class.h"
#include "population/population_type.h"
#include "unit/civilian_unit_class.h"
#include "unit/civilian_unit_type.h"
#include "unit/military_unit_class.h"
#include "unit/military_unit_type.h"
#include "unit/transporter_class.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/string_util.h"

namespace metternich {

culture_base::culture_base(const std::string &identifier) : named_data_entry(identifier)
{
}

culture_base::~culture_base()
{
}

void culture_base::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "title_names") {
		government_type::process_title_name_scope(this->title_names, scope);
	} else if (tag == "ruler_title_names") {
		government_type::process_ruler_title_name_scope(this->ruler_title_names, scope);
	} else if (tag == "building_class_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const building_class *building_class = building_class::get(key);
			const building_type *building_type = building_type::get(value);
			this->set_building_class_type(building_class, building_type);
		});
	} else if (tag == "population_class_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const population_class *population_class = population_class::get(key);
			const population_type *population_type = population_type::get(value);
			this->set_population_class_type(population_class, population_type);
		});
	} else if (tag == "civilian_class_unit_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const civilian_unit_class *unit_class = civilian_unit_class::get(key);
			const civilian_unit_type *unit_type = civilian_unit_type::get(value);
			this->set_civilian_class_unit_type(unit_class, unit_type);
		});
	} else if (tag == "military_class_unit_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const military_unit_class *unit_class = military_unit_class::get(key);
			const military_unit_type *unit_type = military_unit_type::get(value);
			this->set_military_class_unit_type(unit_class, unit_type);
		});
	} else if (tag == "transporter_class_types") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const transporter_class *transporter_class = transporter_class::get(key);
			const transporter_type *transporter_type = transporter_type::get(value);
			this->set_transporter_class_type(transporter_class, transporter_type);
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void culture_base::check() const
{
	for (const auto &[building_class, building_type] : this->building_class_types) {
		assert_throw(building_type->get_building_class() == building_class);
	}

	for (const auto &[population_class, population_type] : this->population_class_types) {
		assert_throw(population_type->get_population_class() == population_class);
	}

	for (const auto &[unit_class, unit_type] : this->civilian_class_unit_types) {
		assert_throw(unit_type->get_unit_class() == unit_class);
	}

	for (const auto &[unit_class, unit_type] : this->military_class_unit_types) {
		assert_throw(unit_type->get_unit_class() == unit_class);
	}
}

data_entry_history *culture_base::get_history_base()
{
	return this->history.get();
}

void culture_base::reset_history()
{
	this->history = make_qunique<culture_history>();
}

phenotype *culture_base::get_default_phenotype() const
{
	if (this->default_phenotype != nullptr) {
		return this->default_phenotype;
	}

	return this->get_group()->get_default_phenotype();
}


bool culture_base::is_part_of_group(const cultural_group *group) const
{
	if (this->get_group() == nullptr) {
		return false;
	}

	if (this->get_group() == group) {
		return true;
	}

	//not the same group, and has a rank lesser than or equal to that of our group, so it can't be an upper group of ours
	if (group->get_rank() <= this->get_group()->get_rank()) {
		return false;
	}

	return this->get_group()->is_part_of_group(group);
}

const std::string &culture_base::get_title_name(const government_type *government_type, const country_tier tier) const
{
	auto find_iterator = this->title_names.find(government_type);
	if (find_iterator == this->title_names.end()) {
		find_iterator = this->title_names.find(government_type->get_group());
	}

	if (find_iterator != this->title_names.end()) {
		const auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator != find_iterator->second.end()) {
			return sub_find_iterator->second;
		}
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_title_name(government_type, tier);
	}

	return string::empty_str;
}

const std::string &culture_base::get_ruler_title_name(const government_type *government_type, const country_tier tier, const gender gender) const
{
	auto find_iterator = this->ruler_title_names.find(government_type);
	if (find_iterator == this->ruler_title_names.end()) {
		find_iterator = this->ruler_title_names.find(government_type->get_group());
	}

	if (find_iterator != this->ruler_title_names.end()) {
		const auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator != find_iterator->second.end()) {
			auto sub_sub_find_iterator = sub_find_iterator->second.find(gender);
			if (sub_sub_find_iterator == sub_find_iterator->second.end()) {
				sub_sub_find_iterator = sub_find_iterator->second.find(gender::none);
			}

			if (sub_sub_find_iterator != sub_find_iterator->second.end()) {
				return sub_sub_find_iterator->second;
			}
		}
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_ruler_title_name(government_type, tier, gender);
	}

	return string::empty_str;
}

const building_type *culture_base::get_building_class_type(const building_class *building_class) const
{
	const auto find_iterator = this->building_class_types.find(building_class);
	if (find_iterator != this->building_class_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_building_class_type(building_class);
	}

	return building_class->get_default_building_type();

}

const population_type *culture_base::get_population_class_type(const population_class *population_class) const
{
	const auto find_iterator = this->population_class_types.find(population_class);
	if (find_iterator != this->population_class_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_population_class_type(population_class);
	}

	return population_class->get_default_population_type();

}

const civilian_unit_type *culture_base::get_civilian_class_unit_type(const civilian_unit_class *unit_class) const
{
	const auto find_iterator = this->civilian_class_unit_types.find(unit_class);
	if (find_iterator != this->civilian_class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_civilian_class_unit_type(unit_class);
	}

	return unit_class->get_default_unit_type();

}

const military_unit_type *culture_base::get_military_class_unit_type(const military_unit_class *unit_class) const
{
	const auto find_iterator = this->military_class_unit_types.find(unit_class);
	if (find_iterator != this->military_class_unit_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_military_class_unit_type(unit_class);
	}

	return unit_class->get_default_unit_type();

}

const transporter_type *culture_base::get_transporter_class_type(const transporter_class *transporter_class) const
{
	const auto find_iterator = this->transporter_class_types.find(transporter_class);
	if (find_iterator != this->transporter_class_types.end()) {
		return find_iterator->second;
	}

	if (this->get_group() != nullptr) {
		return this->get_group()->get_transporter_class_type(transporter_class);
	}

	return transporter_class->get_default_transporter_type();

}

}
