#include "metternich.h"

#include "map/route_game_data.h"

#include "database/defines.h"
#include "database/preferences.h"
#include "economy/commodity.h"
#include "game/game.h"
#include "map/province.h"
#include "map/province_map_data.h"
#include "map/route.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/site_map_data.h"
#include "script/condition/and_condition.h"
#include "util/assert_util.h"
#include "util/point_util.h"
#include "util/string_conversion_util.h"
#include "util/string_util.h"

namespace metternich {

route_game_data::route_game_data(const metternich::route *route) : route(route)
{
	connect(this, &route_game_data::active_changed, game::get(), &game::active_routes_changed);
}

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

	centesimal_int output(this->route->get_output_multiplier());
	output *= total_holding_level;
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

QString route_game_data::get_site_modifier_string() const
{
	std::string str;

	if (this->route->get_output_commodity() != nullptr) {
		const centesimal_int output = this->get_output();
		const std::string number_str = "+" + this->route->get_output_commodity()->value_to_string(output, false);
		const QColor &number_color = defines::get()->get_green_text_color();
		const std::string colored_number_str = string::colored(number_str, number_color);
		str += std::format("{} Output: {}", this->route->get_output_commodity()->get_name(), colored_number_str);
	}

	return QString::fromStdString(str);
}

QString route_game_data::get_line_path() const
{
	if (!this->is_active()) {
		return QString();
	}

	std::vector<QPoint> line_points;

	const QPoint &start_point = this->route->get_start_site() != nullptr ? this->route->get_start_site()->get_map_data()->get_tile_pos() : this->route->get_path_provinces().front()->get_map_data()->get_center_tile_pos();
	const QPoint &end_point = this->route->get_end_site() != nullptr ? this->route->get_end_site()->get_map_data()->get_tile_pos() : this->route->get_path_provinces().back()->get_map_data()->get_center_tile_pos();

	line_points.push_back(start_point);

	//use the holding site locations in the path provinces to form the path
	for (size_t i = 0; i < this->route->get_path_provinces().size(); ++i) {
		const province *path_province = this->route->get_path_provinces().at(i);

		const bool start_province = (path_province == this->route->get_path_provinces().front());
		const bool end_province = (path_province == this->route->get_path_provinces().back());

		const QPoint &next_province_point = end_province ? end_point : this->route->get_path_provinces().at(i + 1)->get_map_data()->get_center_tile_pos();

		std::vector<const site *> province_holding_sites = path_province->get_map_data()->get_settlement_sites();

		bool first = true;

		while (!province_holding_sites.empty()) {
			const QPoint &previous_point = line_points.back();
			const int previous_point_distance = point::distance_to(previous_point, next_province_point);

			std::erase_if(province_holding_sites, [&next_province_point, previous_point_distance](const site *site) {
				return point::distance_to(site->get_map_data()->get_tile_pos(), next_province_point) >= previous_point_distance;
			});

			if (province_holding_sites.empty()) {
				//for the start and end province we shouldn't add the center tile pos if no sites were available that were close enough, since we already add points in them as the start and end points anyway
				if (first && !start_province && !end_province) {
					line_points.push_back(path_province->get_map_data()->get_center_tile_pos());
				}
				break;
			}

			int best_distance_to_previous = std::numeric_limits<int>::max();
			const site *best_holding_site = nullptr;

			for (const site *holding_site : province_holding_sites) {
				const int distance_to_previous = point::distance_to(holding_site->get_map_data()->get_tile_pos(), previous_point);
				if (distance_to_previous < best_distance_to_previous) {
					best_holding_site = holding_site;
					best_distance_to_previous = distance_to_previous;
				}
			}

			assert_throw(best_holding_site != nullptr);

			if (end_province && best_holding_site == this->route->get_end_site()) {
				break;
			}

			line_points.push_back(best_holding_site->get_map_data()->get_tile_pos());
			first = false;
		}
	}

	line_points.push_back(end_point);

	std::string line_path;
	bool first = true;

	for (const QPoint &point : line_points) {
		if (first) {
			line_path += std::format("M {} {}", (point.x() * defines::get()->get_province_map_tile_scale() * preferences::get()->get_scale_factor()).to_int(), (point.y() * defines::get()->get_province_map_tile_scale() * preferences::get()->get_scale_factor()).to_int());
			first = false;
		} else {
			line_path += std::format(" L {} {}", (point.x() * defines::get()->get_province_map_tile_scale() * preferences::get()->get_scale_factor()).to_int(), (point.y() * defines::get()->get_province_map_tile_scale() * preferences::get()->get_scale_factor()).to_int());
		}
	}

	return QString::fromStdString(line_path);
}

}
