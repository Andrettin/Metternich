#include "metternich.h"

#include "map/site_feature.h"

#include "infrastructure/holding_type.h"
#include "map/site.h"
#include "map/site_game_data.h"
#include "map/terrain_type.h"
#include "script/factor.h"
#include "script/modifier.h"

namespace metternich {

site_feature::site_feature(const std::string &identifier) : named_data_entry(identifier)
{
}

site_feature::~site_feature()
{
}

void site_feature::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "holding_types") {
		for (const std::string &value : values) {
			this->holding_types.push_back(holding_type::get(value));
		}
	} else if (tag == "terrain_types") {
		for (const std::string &value : values) {
			this->terrain_types.push_back(terrain_type::get(value));
		}
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const site>>();
		this->modifier->process_gsml_data(scope);
	} else if (tag == "domain_modifier") {
		this->domain_modifier = std::make_unique<metternich::modifier<const domain>>();
		this->domain_modifier->process_gsml_data(scope);
	} else if (tag == "weight_factor") {
		this->weight_factor = std::make_unique<factor<site>>();
		this->weight_factor->process_gsml_data(scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void site_feature::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Site feature \"{}\" has no icon.", this->get_identifier()));
	}

	named_data_entry::check();
}

QString site_feature::get_modifier_string(const metternich::site *site) const
{
	std::string str;

	if (this->get_modifier() != nullptr) {
		str = this->get_modifier()->get_single_line_string(site);
	}

	if (this->get_domain_modifier() != nullptr && site->get_game_data()->get_owner() != nullptr) {
		if (!str.empty()) {
			str += ", ";
		}

		str += this->get_domain_modifier()->get_single_line_string(site->get_game_data()->get_owner());
	}

	return QString::fromStdString(str);
}

}
