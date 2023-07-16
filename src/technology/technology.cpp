#include "metternich.h"

#include "technology/technology.h"

#include "character/character.h"
#include "country/country.h"
#include "country/culture.h"
#include "country/government_type.h"
#include "economy/commodity.h"
#include "economy/production_type.h"
#include "economy/resource.h"
#include "infrastructure/building_type.h"
#include "infrastructure/improvement.h"
#include "infrastructure/pathway.h"
#include "infrastructure/wonder.h"
#include "map/terrain_type.h"
#include "script/condition/condition.h"
#include "script/modifier.h"
#include "technology/technological_period.h"
#include "technology/technology_category.h"
#include "unit/military_unit_domain.h"
#include "unit/military_unit_type.h"
#include "unit/transporter_type.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/log_util.h"
#include "util/vector_util.h"

namespace metternich {
	
void technology::initialize_all()
{
	data_type::initialize_all();

	technology::sort_instances([](const technology *lhs, const technology *rhs) {
		if (lhs->get_category() != rhs->get_category()) {
			return lhs->get_category() < rhs->get_category();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

technology::technology(const std::string &identifier)
	: named_data_entry(identifier), category(technology_category::none)
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

void technology::initialize()
{
	this->calculate_total_prerequisite_depth();

	if (this->get_year() != 0) {
		this->period = technological_period::get_by_year(this->get_year());
	}

	std::sort(this->enabled_pathways.begin(), this->enabled_pathways.end(), pathway_compare());
	std::sort(this->enabled_river_crossing_pathways.begin(), this->enabled_river_crossing_pathways.end(), pathway_compare());

	named_data_entry::initialize();
}

void technology::check() const
{
	if (this->get_category() == technology_category::none) {
		throw std::runtime_error(std::format("Technology \"{}\" has no category.", this->get_identifier()));
	}

	if (this->get_portrait() == nullptr) {
		throw std::runtime_error(std::format("Technology \"{}\" has no portrait.", this->get_identifier()));
	}

	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Technology \"{}\" has no icon.", this->get_identifier()));
	}

	if (this->get_cost() == 0 && !this->is_discovery()) {
		throw std::runtime_error(std::format("Technology \"{}\" has no cost, and is not a discovery.", this->get_identifier()));
	}

	if (this->get_period() != nullptr) {
		if (this->get_period()->get_index() != this->get_total_prerequisite_depth()) {
			log::log_error(std::format("The period for technology \"{}\" ({}-{}, index {}) does not match its total prerequisite depth ({}).", this->get_identifier(), this->get_period()->get_start_year(), this->get_period()->get_end_year(), this->get_period()->get_index(), this->get_total_prerequisite_depth()));
		}
	} else {
		log::log_error(std::format("Technology \"{}\" has no period.", this->get_identifier()));
	}
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

void technology::calculate_total_prerequisite_depth()
{
	if (this->total_prerequisite_depth != 0 || this->get_prerequisites().empty()) {
		return;
	}

	int depth = 0;

	for (technology *prerequisite : this->get_prerequisites()) {
		prerequisite->calculate_total_prerequisite_depth();
		depth = std::max(depth, prerequisite->get_total_prerequisite_depth() + 1);
	}

	this->total_prerequisite_depth = depth;
}

QVariantList technology::get_enabled_buildings_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_buildings());
}

std::vector<const building_type *> technology::get_enabled_buildings_for_culture(const culture *culture) const
{
	std::vector<const building_type *> buildings;

	for (const building_type *building : this->get_enabled_buildings()) {
		if (building != culture->get_building_class_type(building->get_building_class())) {
			continue;
		}

		buildings.push_back(building);
	}

	return buildings;
}

QVariantList technology::get_enabled_improvements_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_improvements());
}

QVariantList technology::get_enabled_pathways_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_pathways());
}

QVariantList technology::get_enabled_military_units_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_military_units());
}

std::vector<const military_unit_type *> technology::get_enabled_military_units_for_culture(const culture *culture) const
{
	std::vector<const military_unit_type *> military_units;

	for (const military_unit_type *military_unit : this->get_enabled_military_units()) {
		assert_throw(military_unit->get_unit_class() != nullptr);

		if (military_unit != culture->get_military_class_unit_type(military_unit->get_unit_class())) {
			continue;
		}

		military_units.push_back(military_unit);
	}

	return military_units;
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

QVariantList technology::get_enabled_transporters_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_transporters());
}

std::vector<const transporter_type *> technology::get_enabled_transporters_for_culture(const culture *culture) const
{
	std::vector<const transporter_type *> transporters;

	for (const transporter_type *transporter : this->get_enabled_transporters()) {
		assert_throw(transporter->get_transporter_class() != nullptr);

		if (transporter != culture->get_transporter_class_type(transporter->get_transporter_class())) {
			continue;
		}

		transporters.push_back(transporter);
	}

	return transporters;
}

void technology::add_enabled_transporter(const transporter_type *transporter)
{
	this->enabled_transporters.push_back(transporter);

	std::sort(this->enabled_transporters.begin(), this->enabled_transporters.end(), [](const transporter_type *lhs, const transporter_type *rhs) {
		if (lhs->get_category() != rhs->get_category()) {
			return lhs->get_category() < rhs->get_category();
		}

		return lhs->get_identifier() < rhs->get_identifier();
	});
}

void technology::add_enabled_government_type(const government_type *government_type)
{
	this->enabled_government_types.push_back(government_type);

	std::sort(this->enabled_government_types.begin(), this->enabled_government_types.end(), [](const metternich::government_type *lhs, const metternich::government_type *rhs) {
		return lhs->get_identifier() < rhs->get_identifier();
	});
}

std::vector<const character *> technology::get_enabled_rulers_for_country(const country *country) const
{
	std::vector<const character *> rulers;

	for (const character *ruler : this->get_enabled_rulers()) {
		if (!vector::contains(country->get_rulers(), ruler)) {
			continue;
		}

		if (ruler->get_conditions() != nullptr && !ruler->get_conditions()->check(country, read_only_context(country))) {
			continue;
		}

		rulers.push_back(ruler);
	}

	return rulers;
}

void technology::add_enabled_ruler(const character *ruler)
{
	this->enabled_rulers.push_back(ruler);

	std::sort(this->enabled_rulers.begin(), this->enabled_rulers.end(), [](const character *lhs, const character *rhs) {
		return lhs->get_full_name() < rhs->get_full_name();
	});
}

std::vector<const character *> technology::get_retired_rulers_for_country(const country *country) const
{
	std::vector<const character *> rulers;

	for (const character *ruler : this->get_retired_rulers()) {
		if (!vector::contains(country->get_rulers(), ruler)) {
			continue;
		}

		if (ruler->get_conditions() != nullptr && !ruler->get_conditions()->check(country, read_only_context(country))) {
			continue;
		}

		rulers.push_back(ruler);
	}

	return rulers;
}

void technology::add_retired_ruler(const character *ruler)
{
	this->retired_rulers.push_back(ruler);

	std::sort(this->retired_rulers.begin(), this->retired_rulers.end(), [](const character *lhs, const character *rhs) {
		return lhs->get_full_name() < rhs->get_full_name();
	});
}

QVariantList technology::get_enabled_advisors_qvariant_list() const
{
	return container::to_qvariant_list(this->get_enabled_advisors());
}

std::vector<const character *> technology::get_enabled_advisors_for_country(const country *country) const
{
	std::vector<const character *> advisors;

	for (const character *advisor : this->get_enabled_advisors()) {
		if (advisor->get_conditions() != nullptr && !advisor->get_conditions()->check(country, read_only_context(country))) {
			continue;
		}

		advisors.push_back(advisor);
	}

	return advisors;
}

void technology::add_enabled_advisor(const character *advisor)
{
	this->enabled_advisors.push_back(advisor);

	std::sort(this->enabled_advisors.begin(), this->enabled_advisors.end(), [](const character *lhs, const character *rhs) {
		return lhs->get_full_name() < rhs->get_full_name();
	});
}

QVariantList technology::get_retired_advisors_qvariant_list() const
{
	return container::to_qvariant_list(this->get_retired_advisors());
}

std::vector<const character *> technology::get_retired_advisors_for_country(const country *country) const
{
	std::vector<const character *> advisors;

	for (const character *advisor : this->get_retired_advisors()) {
		if (advisor->get_conditions() != nullptr && !advisor->get_conditions()->check(country, read_only_context(country))) {
			continue;
		}

		advisors.push_back(advisor);
	}

	return advisors;
}

void technology::add_retired_advisor(const character *advisor)
{
	this->retired_advisors.push_back(advisor);

	std::sort(this->retired_advisors.begin(), this->retired_advisors.end(), [](const character *lhs, const character *rhs) {
		return lhs->get_full_name() < rhs->get_full_name();
	});
}

std::string technology::get_modifier_string(const country *country) const
{
	if (this->get_modifier() == nullptr) {
		return std::string();
	}

	return this->get_modifier()->get_string(country);
}

QString technology::get_effects_string(metternich::country *country) const
{
	std::string str = this->get_modifier_string(country);

	if (this->grants_free_technology()) {
		if (!str.empty()) {
			str += "\n";
		}

		str += "Free technology for the first to research";
	}

	if (!this->get_enabled_commodities().empty()) {
		for (const commodity *commodity : this->get_enabled_commodities()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} commodity", commodity->get_name());
		}
	}

	if (!this->get_enabled_resources().empty()) {
		for (const resource *resource : this->get_enabled_resources()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} resource", resource->get_name());
		}
	}

	const std::vector<const building_type *> buildings = get_enabled_buildings_for_culture(country->get_culture());

	if (!buildings.empty()) {
		for (const building_type *building : buildings) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} building", building->get_name());
		}
	}

	if (!this->get_enabled_wonders().empty()) {
		for (const wonder *wonder : this->get_enabled_wonders()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} wonder", wonder->get_name());
		}
	}

	if (!this->get_disabled_wonders().empty()) {
		for (const wonder *wonder : this->get_disabled_wonders()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Obsoletes {} wonder", wonder->get_name());
		}
	}

	if (!this->get_enabled_production_types().empty()) {
		for (const production_type *production_type : this->get_enabled_production_types()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} production type", production_type->get_name());
		}
	}

	if (!this->get_enabled_improvements().empty()) {
		for (const improvement *improvement : this->get_enabled_improvements()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} improvement", improvement->get_name());
		}
	}

	if (!this->get_enabled_pathways().empty()) {
		for (const pathway *pathway : this->get_enabled_pathways()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {}", pathway->get_name());
		}
	}

	if (!this->get_enabled_river_crossing_pathways().empty()) {
		for (const pathway *pathway : this->get_enabled_river_crossing_pathways()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} across rivers", pathway->get_name());
		}
	}

	if (!this->get_enabled_pathway_terrains().empty()) {
		for (const auto &[pathway, terrains] : this->get_enabled_pathway_terrains()) {
			for (const terrain_type *terrain : terrains) {
				if (!str.empty()) {
					str += "\n";
				}

				str += std::format("Enables {} in {}", pathway->get_name(), terrain->get_name());
			}
		}
	}

	const std::vector<const military_unit_type *> military_units = get_enabled_military_units_for_culture(country->get_culture());

	if (!military_units.empty()) {
		for (const military_unit_type *military_unit : military_units) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} {}", military_unit->get_name(), military_unit->get_domain() == military_unit_domain::water ? "ship" : "regiment");
		}
	}

	const std::vector<const transporter_type *> transporters = get_enabled_transporters_for_culture(country->get_culture());

	if (!transporters.empty()) {
		for (const transporter_type *transporter : transporters) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} {}", transporter->get_name(), transporter->is_ship() ? "ship" : "transporter");
		}
	}

	if (!this->get_enabled_government_types().empty()) {
		for (const government_type *government_type : this->get_enabled_government_types()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} government type", government_type->get_name());
		}
	}

	const std::vector<const character *> enabled_rulers = get_enabled_rulers_for_country(country);

	if (!enabled_rulers.empty()) {
		for (const character *ruler : enabled_rulers) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} ruler", ruler->get_full_name());
		}
	}

	const std::vector<const character *> retired_rulers = get_retired_rulers_for_country(country);

	if (!retired_rulers.empty()) {
		for (const character *ruler : retired_rulers) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Retires {} ruler", ruler->get_full_name());
		}
	}

	const std::vector<const character *> enabled_advisors = get_enabled_advisors_for_country(country);

	if (!enabled_advisors.empty()) {
		for (const character *advisor : enabled_advisors) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Enables {} advisor", advisor->get_full_name());
		}
	}

	const std::vector<const character *> retired_advisors = get_retired_advisors_for_country(country);

	if (!retired_advisors.empty()) {
		for (const character *advisor : retired_advisors) {
			if (!str.empty()) {
				str += "\n";
			}

			str += std::format("Retires {} advisor", advisor->get_full_name());
		}
	}

	return QString::fromStdString(str);
}

}
