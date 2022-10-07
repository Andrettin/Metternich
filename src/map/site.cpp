#include "metternich.h"

#include "map/site.h"

#include "map/site_type.h"
#include "map/world.h"
#include "util/assert_util.h"

namespace metternich {

site::site(const std::string &identifier) : named_data_entry(identifier), type(site_type::settlement)
{
}

void site::initialize()
{
	assert_throw(this->world != nullptr);
	this->world->add_site(this);

	data_entry::initialize();
}

void site::check() const
{
	assert_throw(this->get_geocoordinate().is_valid());

	if (this->get_type() == site_type::terrain) {
		assert_throw(this->get_terrain_type() != nullptr);
	} else {
		assert_throw(this->get_terrain_type() == nullptr);
	}
}

}
