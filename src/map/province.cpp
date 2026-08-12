#include "metternich.h"

#include "map/province.h"

#include "culture/cultural_group.h"
#include "culture/culture.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "map/province_feature.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/province_map_data.h"
#include "map/province_turn_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/site_feature.h"
#include "map/terrain_type.h"
#include "map/world.h"
#include "util/assert_util.h"
#include "util/container_util.h"
#include "util/log_util.h"
#include "util/vector_util.h"

namespace metternich {

const std::set<std::string> province::database_dependencies = {
	region::class_identifier
};

void province::initialize_all()
{
	data_type::initialize_all();

	std::map<const metternich::world *, world::province_geodata_map_type> world_province_geodata_maps;

	for (province *province : province::get_all()) {
		if (province->get_world() != nullptr && province->uses_geopolygons()) {
			if (!world_province_geodata_maps.contains(province->get_world())) {
				world_province_geodata_maps[province->get_world()] = province->get_world()->parse_provinces_geojson_folder();
			}

			const auto world_find_iterator = world_province_geodata_maps.find(province->get_world());
			const auto province_find_iterator = world_find_iterator->second.find(province);

			province->geopolygons = std::move(province_find_iterator->second);
		}
	}
}

province::province(const std::string &identifier) : named_data_entry(identifier)
{
	this->reset_map_data();
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
		scope.for_each_property([this](const gsml_property &property) {
			const culture_base *culture = cultural_group::try_get(property.get_key());
			if (culture == nullptr) {
				culture = culture::get(property.get_key());
			}
			this->cultural_names[culture] = property.get_value();
		});
	} else if (tag == "features") {
		for (const std::string &value : values) {
			this->features.push_back(province_feature::get(value));
		}
	} else if (tag == "resource_counts") {
		scope.for_each_property([this](const gsml_property &property) {
			const site_feature *resource = site_feature::get(property.get_key());
			this->resource_counts[resource] = std::stoi(property.get_value());
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
	if (this->get_terrain() == nullptr && !this->is_water_zone() && this->get_primary_star() == nullptr) {
		log::log_error(std::format("Land province \"{}\" has no terrain.", this->get_identifier()));
	}

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

	for (const province_feature *feature : this->get_features()) {
		if (!this->can_have_feature(feature)) {
			throw std::runtime_error(std::format("Province \"{}\" has the \"{}\" feature predefined for it, but the province does not fulfill the feature's requirements.", this->get_identifier(), feature->get_identifier()));
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

QCoro::Task<void> province::reset_game_data()
{
	this->game_data = make_qunique<province_game_data>(this);
	co_await this->get_game_data()->initialize();

	this->reset_turn_data();
}

void province::reset_turn_data()
{
	this->turn_data = make_qunique<province_turn_data>(this);
	emit turn_data_changed();
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

const std::string &province::get_cultural_name(const culture_base *culture) const
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

bool province::can_have_feature(const province_feature *feature) const
{
	if (!feature->get_terrain_types().empty()) {
		if (this->get_terrain() != nullptr && !vector::contains(feature->get_terrain_types(), this->get_terrain())) {
			return false;
		}
	}

	return true;
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

QVariantList province::get_routes_qvariant_list() const
{
	return container::to_qvariant_list(this->get_routes());
}

}
