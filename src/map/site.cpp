#include "metternich.h"

#include "map/site.h"

#include "country/cultural_group.h"
#include "country/culture.h"
#include "map/province.h"
#include "map/province_history.h"
#include "map/site_game_data.h"
#include "map/site_history.h"
#include "map/site_type.h"
#include "map/tile.h"
#include "map/world.h"
#include "util/assert_util.h"
#include "util/log_util.h"

namespace metternich {

site::site(const std::string &identifier) : named_data_entry(identifier), type(site_type::none)
{
	this->reset_game_data();
}

site::~site()
{
}

void site::process_gsml_scope(const gsml_data &scope)
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
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void site::initialize()
{
	assert_throw(this->world != nullptr);
	this->world->add_site(this);

	if (this->get_province() != nullptr) {
		this->province->add_site(this);
	}

	named_data_entry::initialize();
}

void site::check() const
{
	if (this->get_type() == site_type::terrain) {
		assert_throw(this->get_terrain_type() != nullptr);
	} else {
		if (this->get_type() != site_type::resource && this->get_type() != site_type::settlement) { //resource and settlement sites can also have a terrain type
			assert_throw(this->get_terrain_type() == nullptr);
		}
	}

	if (this->get_type() == site_type::resource) {
		assert_throw(this->get_resource() != nullptr);

		if (this->get_province() == nullptr && this->get_geocoordinate().is_valid()) {
			log::log_error("Resource site \"" + this->get_identifier() + "\" has no province, but has a valid geocoordinate.");
		}
	} else {
		assert_throw(this->get_resource() == nullptr);
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

void site::reset_game_data()
{
	this->game_data = make_qunique<site_game_data>(this);
}

bool site::is_settlement() const
{
	return this->get_type() == site_type::settlement;
}

const std::string &site::get_cultural_name(const culture *culture) const
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

}
