#include "metternich.h"

#include "map/province.h"

#include "domain/cultural_group.h"
#include "domain/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/resource.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/province_map_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_type.h"
#include "map/terrain_feature.h"
#include "map/terrain_type.h"
#include "map/world.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/vector_util.h"

namespace metternich {

const std::set<std::string> province::database_dependencies = {
	region::class_identifier
};

province::province(const std::string &identifier) : named_data_entry(identifier)
{
	this->reset_map_data();
	this->reset_game_data();
}

province::~province()
{
}

void province::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "terrain_types") {
		for (const std::string &value : values) {
			this->terrain_types.push_back(terrain_type::get(value));
		}
	} else if (tag == "cultural_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const culture *culture = culture::get(property.get_key());
			this->cultural_names[culture] = property.get_value();
		});
	} else if (tag == "cultural_group_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const cultural_group *cultural_group = cultural_group::get(property.get_key());
			this->cultural_group_names[cultural_group] = property.get_value();
		});
	} else if (tag == "border_rivers") {
		scope.for_each_property([&](const gsml_property &property) {
			province *border_province = province::get(property.get_key());
			const terrain_feature *border_river = terrain_feature::get(property.get_value());
			this->border_rivers[border_province] = border_river;
			border_province->border_rivers[this] = border_river;
		});
	} else if (tag == "generation_worlds") {
		for (const std::string &value : values) {
			this->generation_worlds.push_back(world::get(value));
		}
	} else {
		named_data_entry::process_gsml_scope(scope);
	}
}

void province::initialize()
{
	if (this->get_default_provincial_capital() != nullptr) {
		assert_throw(this->get_default_provincial_capital()->get_province() == nullptr || this->get_default_provincial_capital()->get_province() == this);

		this->default_provincial_capital->set_province(this);

		if (this->default_provincial_capital->is_initialized()) {
			//site is already initialized, so it won't add itself to this province's site list
			this->add_site(this->default_provincial_capital);
		}
	}

	if (this->get_primary_star() != nullptr) {
		assert_throw(this->get_primary_star()->get_province() == nullptr || this->get_primary_star()->get_province() == this);

		this->primary_star->set_province(this);

		if (this->primary_star->is_initialized()) {
			//site is already initialized, so it won't add itself to this province's site list
			this->add_site(this->primary_star);
		}
	}

	if (this->get_world() == nullptr) {
		for (const region *region : this->get_regions()) {
			if (region->get_world() != nullptr) {
				this->world = region->get_world();
				break;
			}
		}
	}

	named_data_entry::initialize();
}

void province::check() const
{
	if (this->get_default_provincial_capital() == nullptr && !this->is_water_zone()) {
		throw std::runtime_error(std::format("Province \"{}\" has no default provincial capital.", this->get_identifier()));
	} else if (this->get_default_provincial_capital() != nullptr && this->is_water_zone()) {
		throw std::runtime_error(std::format("Water zone \"{}\" has a default provincial capital.", this->get_identifier()));
	}

	if (this->get_default_provincial_capital() != nullptr && !this->get_default_provincial_capital()->is_settlement()) {
		throw std::runtime_error(std::format("Province \"{}\" has a default provincial capital (\"{}\") which is not a settlement.", this->get_identifier(), this->get_default_provincial_capital()->get_identifier()));
	}

	if (this->get_primary_star() != nullptr && !this->get_primary_star()->is_celestial_body()) {
		throw std::runtime_error(std::format("Province \"{}\" has a primary star (\"{}\") which is not a celestial body.", this->get_identifier(), this->get_primary_star()->get_identifier()));
	}

	for (const auto &[border_province, border_river] : this->border_rivers) {
		if (!border_river->is_river() && !border_river->is_border_river()) {
			throw std::runtime_error(std::format("Province \"{}\" has the terrain feature \"{}\" set as a border river, but the latter is not a river.", this->get_identifier(), border_river->get_identifier()));
		}
	}
}

data_entry_history *province::get_history_base()
{
	return this->history.get();
}

void province::reset_history()
{
	this->history = make_qunique<province_history>(this);
}

void province::reset_map_data()
{
	this->map_data = make_qunique<province_map_data>(this);
}

void province::reset_game_data()
{
	this->game_data = make_qunique<province_game_data>(this);
}

const geocoordinate &province::get_geocoordinate() const
{
	if (this->get_default_provincial_capital() != nullptr) {
		return this->get_default_provincial_capital()->get_geocoordinate();
	}

	return this->geocoordinate;
}

std::string province::get_scope_name() const
{
	return this->get_game_data()->get_current_cultural_name();
}

const std::string &province::get_cultural_name(const culture *culture) const
{
	if (culture != nullptr) {
		const auto find_iterator = this->cultural_names.find(culture);
		if (find_iterator != this->cultural_names.end()) {
			return find_iterator->second;
		}

		if (culture->get_group() != nullptr) {
			return this->get_cultural_name(culture->get_group());
		}
	}

	return this->get_name();
}

const std::string &province::get_cultural_name(const cultural_group *cultural_group) const
{
	if (cultural_group != nullptr) {
		const auto group_find_iterator = this->cultural_group_names.find(cultural_group);
		if (group_find_iterator != this->cultural_group_names.end()) {
			return group_find_iterator->second;
		}

		if (cultural_group->get_upper_group() != nullptr) {
			return this->get_cultural_name(cultural_group->get_upper_group());
		}
	}

	return this->get_name();
}

void province::add_region(region *region)
{
	if (!vector::contains(this->regions, region)) {
		this->regions.push_back(region);
	}

	region->add_province(this);
}

void province::remove_region(region *region)
{
	std::erase(this->regions, region);
	region->remove_province(this);
}

std::vector<const region *> province::get_shared_regions_with(const province *other_province) const
{
	return vector::intersected<region *, std::vector<const region *>>(this->get_regions(), other_province->get_regions());
}

bool province::has_core_country_of_culture(const culture *culture) const
{
	for (const domain *domain : this->get_core_countries()) {
		if (domain->get_game_data()->get_culture() == culture) {
			return true;
		}
	}

	return false;
}

}
