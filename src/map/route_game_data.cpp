#include "metternich.h"

#include "map/route_game_data.h"

#include "map/province.h"
#include "map/province_map_data.h"
#include "map/route.h"
#include "util/assert_util.h"
#include "util/string_conversion_util.h"

namespace metternich {

void route_game_data::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "active") {
		this->active = string::to_bool(value);
	} else {
		throw std::runtime_error(std::format("Invalid route game data property: \"{}\".", key));
	}
}

void route_game_data::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	throw std::runtime_error(std::format("Invalid route game data scope: \"{}\".", tag));
}

gsml_data route_game_data::to_gsml_data() const
{
	gsml_data data(this->route->get_identifier());

	assert_throw(this->is_on_map());

	data.add_property("active", string::from_bool(this->is_active()));

	return data;
}

bool route_game_data::is_on_map() const
{
	if (this->route->get_path_provinces().empty()) {
		return false;
	}

	for (const province *province : this->route->get_path_provinces()) {
		if (!province->get_map_data()->is_on_map()) {
			return false;
		}
	}

	return true;
}

void route_game_data::set_active(const bool active)
{
	if (active == this->is_active()) {
		return;
	}

	this->active = active;
}

void route_game_data::check_active()
{
	if (!this->is_on_map()) {
		return;
	}

	//FIXME: check if conditions are fulfilled for each province

	this->set_active(true);
}

}
