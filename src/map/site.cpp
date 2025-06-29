#include "metternich.h"

#include "map/site.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "country/government_type.h"
#include "economy/resource.h"
#include "map/province.h"
#include "map/province_history.h"
#include "map/region.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/site_map_data.h"
#include "map/site_tier.h"
#include "map/site_type.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/world.h"
#include "util/assert_util.h"
#include "util/gender.h"
#include "util/log_util.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

site::site(const std::string &identifier) : named_data_entry(identifier), type(site_type::none), max_tier(site_tier::barony)
{
	this->reset_map_data();
	this->reset_game_data();
}

site::~site()
{
}

void site::process_gsml_scope(const gsml_data &scope)
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
	} else if (tag == "landholder_title_names") {
		government_type::process_landholder_title_name_scope(this->landholder_title_names, scope);
	} else if (tag == "generation_regions") {
		for (const std::string &value : values) {
			this->generation_regions.push_back(region::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void site::initialize()
{
	assert_throw(this->world != nullptr || this->is_celestial_body());
	if (this->world != nullptr) {
		this->world->add_site(this);
	}

	if (this->get_province() != nullptr) {
		this->province->add_site(this);
	}

	if (this->terrain_types.empty() && this->get_resource() != nullptr) {
		this->terrain_types = this->get_resource()->get_terrain_types();
	}

	named_data_entry::initialize();
}

void site::check() const
{
	if (this->get_type() == site_type::terrain) {
		assert_throw(this->get_terrain_type() != nullptr);
	} else {
		if (this->get_type() != site_type::resource && this->get_type() != site_type::settlement) {
			//resource and settlement sites can also have a terrain type
			assert_throw(this->get_terrain_type() == nullptr);
		}
	}

	switch (this->get_type()) {
		case site_type::settlement:
		case site_type::habitable_world:
			if (this->get_resource() == nullptr) {
				log::log_error(std::format("Settlement site \"{}\" has no resource.", this->get_identifier()));
			}

			if (this->get_province() != nullptr) {
				if (this->get_province()->get_provincial_capital() != this) {
					log::log_error(std::format("Settlement site \"{}\" is not the provincial capital of its province.", this->get_identifier()));
				}
			} else {
				log::log_error(std::format("Settlement site \"{}\" has no province.", this->get_identifier()));
			}
			break;
		case site_type::resource:
			if (this->get_resource() == nullptr) {
				throw std::runtime_error(std::format("Resource site \"{}\" has no resource.", this->get_identifier()));
			}
			break;
		default:
			assert_throw(this->get_resource() == nullptr);
			break;
	}

	if (this->get_resource() != nullptr && !this->get_resource()->get_site_types().contains(this->get_type())) {
		throw std::runtime_error(std::format("Site \"{}\" has resource \"{}\", but the latter cannot be set for the site's type (\"{}\").", this->get_identifier(), this->get_resource()->get_identifier(), magic_enum::enum_name(this->get_type())));
	}

	if (this->is_celestial_body() && this->get_celestial_body_type() == nullptr) {
		throw std::runtime_error(std::format("Site \"{}\" is a celestial body, but has no celestial body type.", this->get_identifier()));
	}
}

data_entry_history *site::get_history_base()
{
	return this->history.get();
}

void site::reset_history()
{
	this->history = make_qunique<site_history>(this);
}

void site::reset_map_data()
{
	this->map_data = make_qunique<site_map_data>(this);
}

void site::reset_game_data()
{
	this->game_data = make_qunique<site_game_data>(this);
	this->get_game_data()->initialize_resource();
}

bool site::is_settlement() const
{
	return this->get_type() == site_type::settlement || this->get_type() == site_type::habitable_world;
}

bool site::is_celestial_body() const
{
	return this->get_type() == site_type::celestial_body || this->get_type() == site_type::habitable_world;
}

std::string site::get_scope_name() const
{
	return this->get_game_data()->get_current_cultural_name();
}

const std::string &site::get_cultural_name(const culture *culture) const
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

const std::string &site::get_cultural_name(const cultural_group *cultural_group) const
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

const std::string &site::get_landholder_title_name(const government_type *government_type, const site_tier tier, const gender gender, const culture *culture) const
{
	auto find_iterator = this->landholder_title_names.find(government_type);
	if (find_iterator == this->landholder_title_names.end()) {
		find_iterator = this->landholder_title_names.find(government_type->get_group());
	}

	if (find_iterator != this->landholder_title_names.end()) {
		auto sub_find_iterator = find_iterator->second.find(tier);
		if (sub_find_iterator == find_iterator->second.end()) {
			sub_find_iterator = find_iterator->second.find(site_tier::none);
		}

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

	if (culture != nullptr) {
		const std::string &culture_landholder_title_name = culture->get_landholder_title_name(government_type, tier, gender);
		if (!culture_landholder_title_name.empty()) {
			return culture_landholder_title_name;
		}
	}

	assert_throw(government_type != nullptr);

	return government_type->get_landholder_title_name(tier, gender);
}

bool site::can_be_generated_on_world(const metternich::world *world) const
{
	//whether the site can be generated on a given world other than its own
	for (const region *region : this->get_generation_regions()) {
		if (region->get_world() == world) {
			return true;
		}
	}

	return false;
}

}
