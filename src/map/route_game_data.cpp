#include "metternich.h"

#include "map/route_game_data.h"

#include "map/province.h"
#include "map/province_map_data.h"
#include "map/route.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "script/condition/and_condition.h"
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

	if (this->route->get_start_site() != nullptr && !this->route->get_start_site()->get_map_data()->is_on_map()) {
		return false;
	}

	if (this->route->get_end_site() != nullptr && !this->route->get_end_site()->get_map_data()->is_on_map()) {
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

	if (active) {
		this->apply_output(1);
	} else {
		this->apply_output(-1);
	}

	emit active_changed();
}

void route_game_data::check_active()
{
	if (!this->is_on_map()) {
		return;
	}

	if (this->route->get_start_site() != nullptr && !this->route->get_start_site()->get_game_data()->is_built()) {
		this->set_active(false);
		return;
	}

	if (this->route->get_end_site() != nullptr && !this->route->get_end_site()->get_game_data()->is_built()) {
		this->set_active(false);
		return;
	}

	if (this->route->get_conditions() != nullptr) {
		for (const province *province : this->route->get_path_provinces()) {
			if (!this->route->get_conditions()->check(province, read_only_context(province))) {
				this->set_active(false);
				return;
			}
		}
	}

	this->set_active(true);
}

centesimal_int route_game_data::get_output() const
{
	if (this->route->get_start_site() == nullptr || this->route->get_end_site() == nullptr) {
		return centesimal_int(0);
	}

	const int total_holding_level = this->route->get_start_site()->get_game_data()->get_holding_level() + this->route->get_end_site()->get_game_data()->get_holding_level();

	centesimal_int output(total_holding_level);
	output /= 2; //average of the holding levels of the two connected sites
	output /= 2; //the half-share for each of the two connected sites
	return output;
}

void route_game_data::apply_output(const int multiplier)
{
	if (this->route->get_start_site() == nullptr || this->route->get_end_site() == nullptr) {
		return;
	}

	const centesimal_int output = this->get_output();
	if (output == 0) {
		return;
	}

	assert_throw(this->route->get_output_commodity() != nullptr);

	this->route->get_start_site()->get_game_data()->change_base_commodity_output(this->route->get_output_commodity(), output * multiplier);
	this->route->get_end_site()->get_game_data()->change_base_commodity_output(this->route->get_output_commodity(), output * multiplier);
}

}
