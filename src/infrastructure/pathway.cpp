#include "metternich.h"

#include "infrastructure/pathway.h"

#include "database/database.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "map/terrain_type.h"
#include "map/tile.h"
#include "map/tile_image_provider.h"
#include "script/modifier.h"
#include "technology/technology.h"
#include "util/assert_util.h"
#include "util/vector_util.h"

namespace metternich {

pathway::pathway(const std::string &identifier) : named_data_entry(identifier)
{
}

pathway::~pathway()
{
}

void pathway::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "wealth_cost") {
		this->wealth_cost = defines::get()->get_wealth_commodity()->string_to_value(value);
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

void pathway::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "terrain_required_technologies") {
		scope.for_each_property([this](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();
			const terrain_type *terrain = terrain_type::get(key);
			technology *technology = technology::get(value);

			this->terrain_required_technologies[terrain] = technology;

			technology->add_enabled_pathway_terrain(this, terrain);
		});
	} else if (tag == "commodity_costs") {
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = commodity->string_to_value(property.get_value());
		});
	} else if (tag == "commodity_cost_weights") {
		scope.for_each_property([this](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_cost_weights[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "modifier") {
		auto modifier = std::make_unique<metternich::modifier<const province>>();
		modifier->process_gsml_data(scope);
		this->modifier = std::move(modifier);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void pathway::initialize()
{
	if (this->wealth_cost != 0) {
		assert_throw(this->commodity_costs.empty());

		if (this->commodity_cost_weights.empty()) {
			this->commodity_cost_weights[defines::get()->get_construction_commodity()] = 1;
		}

		int64_t total_weight = 0;
		for (const auto &[commodity, cost_weight] : this->commodity_cost_weights) {
			total_weight += cost_weight;
		}
		for (const auto &[commodity, cost_weight] : this->commodity_cost_weights) {
			assert_throw(commodity->get_base_price() > 0);
			this->commodity_costs[commodity] = this->wealth_cost * cost_weight / total_weight / commodity->get_base_price();
		}

		this->commodity_cost_weights.clear();
		this->wealth_cost = 0;
	}

	if (this->required_technology != nullptr) {
		this->required_technology->add_enabled_pathway(this);
	}

	if (this->river_crossing_required_technology != nullptr) {
		this->river_crossing_required_technology->add_enabled_river_crossing_pathway(this);
	}

	QTimer::singleShot(0, [this]() -> QCoro::Task<void> {
		co_await tile_image_provider::get()->load_image("pathway/" + this->get_identifier() + "/0");
	});

	named_data_entry::initialize();
}

void pathway::check() const
{
	assert_throw(this->get_icon() != nullptr);
	assert_throw(this->get_transport_level() > 0);

	assert_log(!this->get_image_filepath().empty());
}

void pathway::set_image_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_image_filepath()) {
		return;
	}

	this->image_filepath = database::get()->get_graphics_path(this->get_module()) / filepath;
}

commodity_map<int64_t> pathway::get_commodity_costs_for_province(const province *province) const
{
	commodity_map<int64_t> costs = this->get_commodity_costs();

	//multiply the costs by the province's terrain type movement cost
	for (auto &[commodity, cost] : costs) {
		cost *= province->get_game_data()->get_terrain()->get_movement_cost();
	}

	return costs;
}

QString pathway::get_commodity_costs_string_for_province(const metternich::province *province, const bool single_line) const
{
	std::string str;

	const commodity_map<int64_t> commodity_costs = this->get_commodity_costs_for_province(province);

	for (const auto &[commodity, cost] : commodity_costs) {
		if (cost == 0) {
			continue;
		}

		if (str.empty()) {
			str = "Costs:";
			if (single_line) {
				str += " ";
			} else {
				str += "\n";
			}
		} else {
			if (single_line) {
				str += ", ";
			} else {
				str += "\n";
			}
		}

		str += commodity->value_to_string(cost, commodity != defines::get()->get_wealth_commodity());
		if (commodity != defines::get()->get_wealth_commodity()) {
			str += " " + commodity->get_name();
		}
	}

	return QString::fromStdString(str);
}

bool pathway::is_buildable_in_province(const province *province) const
{
	const pathway *province_pathway = province->get_game_data()->get_pathway();

	if (this->get_required_pathway() != nullptr && province_pathway != this->get_required_pathway()) {
		return false;
	}

	if (this->get_required_technology() != nullptr && !province->get_game_data()->has_technology(this->get_required_technology())) {
		return false;
	}

	const technology *terrain_required_technology = this->get_terrain_required_technology(province->get_game_data()->get_terrain());
	if (terrain_required_technology != nullptr && !province->get_game_data()->has_technology(terrain_required_technology)) {
		return false;
	}

	if (province_pathway != nullptr) {
		if (this == province_pathway) {
			return false;
		}

		if (this->get_transport_level() < province_pathway->get_transport_level()) {
			return false;
		}

		if (this->get_transport_level() == province_pathway->get_transport_level()) {
			//the pathway must be better in some way
			return false;
		}
	}

	return true;
}

QString pathway::get_modifier_string(const province *province, const bool single_line) const
{
	assert_throw(province != nullptr);

	std::string str;

	if (this->get_modifier() != nullptr) {
		str = single_line ? this->get_modifier()->get_single_line_string(province) : this->get_modifier()->get_string(province);
	}

	return QString::fromStdString(str);
}

}
