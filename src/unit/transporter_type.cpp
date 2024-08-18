#include "metternich.h"

#include "unit/transporter_type.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "economy/commodity.h"
#include "technology/technology.h"
#include "unit/transporter_category.h"
#include "unit/transporter_class.h"
#include "unit/transporter_stat.h"
#include "util/assert_util.h"

namespace metternich {

void transporter_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "stats") {
		scope.for_each_property([&](const gsml_property &property) {
			const transporter_stat stat = enum_converter<transporter_stat>::to_enum(property.get_key());
			const centesimal_int stat_value(property.get_value());
			this->stats[stat] = stat_value;
		});
	} else if (tag == "commodity_costs") {
		scope.for_each_property([&](const gsml_property &property) {
			const commodity *commodity = commodity::get(property.get_key());
			this->commodity_costs[commodity] = std::stoi(property.get_value());
		});
	} else if (tag == "upgrades") {
		for (const std::string &value : values) {
			this->upgrades.push_back(transporter_type::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void transporter_type::initialize()
{
	assert_throw(this->transporter_class != nullptr);

	this->transporter_class->add_transporter_type(this);

	if (this->culture != nullptr) {
		this->culture->set_transporter_class_type(this->get_transporter_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_transporter_class_type(this->get_transporter_class()) == nullptr);

		this->cultural_group->set_transporter_class_type(this->get_transporter_class(), this);
	} else {
		this->transporter_class->set_default_transporter_type(this);
	}

	if (this->required_technology != nullptr) {
		assert_throw(this->get_transporter_class() != nullptr);
		this->required_technology->add_enabled_transporter(this);
	}

	named_data_entry::initialize();
}

void transporter_type::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

transporter_category transporter_type::get_category() const
{
	if (this->get_transporter_class() == nullptr) {
		return transporter_category::none;
	}

	return this->get_transporter_class()->get_category();
}

bool transporter_type::is_ship() const
{
	return this->get_transporter_class()->is_ship();
}

centesimal_int transporter_type::get_stat_for_country(const transporter_stat stat, const country *country) const
{
	centesimal_int value = this->get_stat(stat);
	value += country->get_game_data()->get_transporter_type_stat_modifier(this, stat);
	return value;
}

int transporter_type::get_score() const
{
	int score = this->get_wealth_cost();

	for (const auto &[commodity, cost] : this->get_commodity_costs()) {
		if (commodity->get_base_price() == 0) {
			continue;
		}

		score += cost * commodity->get_base_price();
	}

	return score;
}

}
