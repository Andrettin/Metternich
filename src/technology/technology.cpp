#include "metternich.h"

#include "technology/technology.h"

#include "character/character.h"
#include "country/culture.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "script/condition/condition.h"
#include "script/modifier.h"
#include "technology/technology_category.h"
#include "unit/military_unit_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/vector_util.h"

namespace metternich {

technology::technology(const std::string &identifier) : named_data_entry(identifier)
{
}

technology::~technology()
{
}

void technology::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "prerequisites") {
		for (const std::string &value : values) {
			this->prerequisites.push_back(technology::get(value));
		}
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const country>>();
		database::process_gsml_data(modifier, scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void technology::check() const
{
	assert_throw(this->get_category() != technology_category::none);
	assert_throw(this->get_portrait() != nullptr);
}

QVariantList technology::get_prerequisites_qvariant_list() const
{
	return container::to_qvariant_list(this->get_prerequisites());
}

bool technology::requires_technology(const technology *technology) const
{
	assert_throw(this != technology);

	for (const metternich::technology *prerequisite : this->get_prerequisites()) {
		if (prerequisite == technology || prerequisite->requires_technology(technology)) {
			return true;
		}
	}

	return false;
}

int technology::get_total_prerequisite_depth() const
{
	int depth = 0;

	for (const technology *prerequisite : this->get_prerequisites()) {
		depth = std::max(depth, prerequisite->get_total_prerequisite_depth() + 1);
	}

	return depth;
}

QVariantList technology::get_enabled_buildings_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_buildings());
}

QVariantList technology::get_enabled_buildings_for_culture(culture *culture) const
{
	std::vector<const building_type *> buildings;

	for (const building_type *building : this->get_enabled_buildings()) {
		if (building != culture->get_building_class_type(building->get_building_class())) {
			continue;
		}

		buildings.push_back(building);
	}

	return container::to_qvariant_list(buildings);
}

QVariantList technology::get_enabled_improvements_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_improvements());
}

QVariantList technology::get_enabled_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_military_units());
}

QVariantList technology::get_enabled_military_units_for_culture(culture *culture) const
{
	std::vector<const military_unit_type *> military_units;

	for (const military_unit_type *military_unit : this->get_enabled_military_units()) {
		assert_throw(military_unit->get_unit_class() != nullptr);

		if (military_unit != culture->get_military_class_unit_type(military_unit->get_unit_class())) {
			continue;
		}

		military_units.push_back(military_unit);
	}

	return container::to_qvariant_list(military_units);
}

void technology::add_enabled_military_unit(const military_unit_type *military_unit)
{
	this->enabled_military_units.push_back(military_unit);

	std::sort(this->enabled_military_units.begin(), this->enabled_military_units.end(), [](const military_unit_type *lhs, const military_unit_type *rhs) {
		if (lhs->get_category() != rhs->get_category()) {
			return lhs->get_category() < rhs->get_category();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

QString technology::get_modifier_string() const
{
	if (this->get_modifier() == nullptr) {
		return QString();
	}

	return QString::fromStdString(this->get_modifier()->get_string());
}

}
