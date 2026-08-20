#include "metternich.h"

#include "map/province_feature.h"

#include "map/province.h"
#include "map/province_game_data.h"
#include "map/terrain_type.h"
#include "script/modifier.h"

namespace metternich {

province_feature::province_feature(const std::string &identifier) : named_data_entry(identifier)
{
}

province_feature::~province_feature()
{
}

void province_feature::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "terrain_types") {
		for (const std::string &value : values) {
			this->terrain_types.push_back(terrain_type::get(value));
		}
	} else if (tag == "modifier") {
		this->modifier = std::make_unique<metternich::modifier<const province>>();
		this->modifier->process_gsml_data(scope);
	} else if (tag == "domain_modifier") {
		this->domain_modifier = std::make_unique<metternich::modifier<const domain>>();
		this->domain_modifier->process_gsml_data(scope);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void province_feature::check() const
{
	if (this->get_icon() == nullptr) {
		throw std::runtime_error(std::format("Province feature \"{}\" has no icon.", this->get_identifier()));
	}

	named_data_entry::check();
}

QString province_feature::get_modifier_string(const metternich::province *province) const
{
	std::string str;

	if (this->get_modifier() != nullptr) {
		str = this->get_modifier()->get_single_line_string(province);
	}

	if (this->get_domain_modifier() != nullptr && province->get_game_data()->get_owner() != nullptr) {
		if (!str.empty()) {
			str += ", ";
		}

		str += this->get_domain_modifier()->get_single_line_string(province->get_game_data()->get_owner());
	}

	return QString::fromStdString(str);
}

}
