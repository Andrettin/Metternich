#include "metternich.h"

#include "map/province.h"

#include "country/country.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "map/province_game_data.h"
#include "map/province_history.h"
#include "map/province_map_data.h"
#include "map/region.h"
#include "map/site.h"
#include "map/terrain_feature.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/vector_util.h"

namespace metternich {

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

	if (tag == "cultural_names") {
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
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void province::initialize()
{
	if (this->get_provincial_capital() != nullptr) {
		assert_throw(this->get_provincial_capital()->get_province() == nullptr || this->get_provincial_capital()->get_province() == this);

		this->provincial_capital->set_province(this);
	}

	named_data_entry::initialize();
}

void province::check() const
{
	if (this->get_provincial_capital() == nullptr && !this->is_water_zone()) {
		throw std::runtime_error(std::format("Province \"{}\" has no provincial capital.", this->get_identifier()));
	} else if (this->get_provincial_capital() != nullptr && this->is_water_zone()) {
		throw std::runtime_error("Water zone \"" + this->get_identifier() + "\" has a provincial capital.");
	}

	if (this->get_provincial_capital() != nullptr && !this->get_provincial_capital()->is_settlement()) {
		throw std::runtime_error(std::format("Province \"{}\" has a provincial capital (\"{}\") which is not a settlement.", this->get_identifier(), this->get_provincial_capital()->get_identifier()));
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

		const auto group_find_iterator = this->cultural_group_names.find(culture->get_group());
		if (group_find_iterator != this->cultural_group_names.end()) {
			return group_find_iterator->second;
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

bool province::has_core_country_of_culture(const culture *culture) const
{
	for (const country *country : this->get_core_countries()) {
		if (country->get_culture() == culture) {
			return true;
		}
	}

	return false;
}

}
