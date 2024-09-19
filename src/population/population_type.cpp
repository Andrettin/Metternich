#include "metternich.h"

#include "population/population_type.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "country/cultural_group.h"
#include "country/culture.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "population/phenotype.h"
#include "population/population.h"
#include "population/population_class.h"
#include "population/profession.h"
#include "script/modifier.h"
#include "ui/icon.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"
#include "util/string_util.h"

namespace metternich {

population_type::population_type(const std::string &identifier) : named_data_entry(identifier)
{
}

population_type::~population_type()
{
}

void population_type::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "everyday_consumption") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const commodity *commodity = commodity::get(key);
			const centesimal_int consumption(value);
			this->everyday_consumption[commodity] = std::move(consumption);
		});
	} else if (tag == "luxury_consumption") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const commodity *commodity = commodity::get(key);
			const centesimal_int consumption(value);
			this->luxury_consumption[commodity] = std::move(consumption);
		});
	} else if (tag == "commodity_demands") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const commodity *commodity = commodity::get(key);
			const centesimal_int demand(value);
			this->commodity_demands[commodity] = std::move(demand);
		});
	} else if (tag == "profession_output_bonuses") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const profession *profession = profession::get(key);
			const centesimal_int output_bonus(value);
			this->profession_output_bonuses[profession] = std::move(output_bonus);
		});
	} else if (tag == "profession_output_modifiers") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const profession *profession = profession::get(key);
			const int output_modifier = std::stoi(value);
			this->profession_output_modifiers[profession] = output_modifier;
		});
	} else if (tag == "country_modifier") {
		this->country_modifier = std::make_unique<modifier<const country>>();
		database::process_gsml_data(this->country_modifier, scope);
	} else if (tag == "phenotype_icons") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const phenotype *phenotype = phenotype::get(key);
			const metternich::icon *icon = icon::get(value);
			this->phenotype_icons[phenotype] = icon;
		});
	} else if (tag == "phenotype_small_icons") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			const phenotype *phenotype = phenotype::get(key);
			const metternich::icon *icon = icon::get(value);
			this->phenotype_small_icons[phenotype] = icon;
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void population_type::initialize()
{
	assert_throw(this->population_class != nullptr);
	this->population_class->add_population_type(this);

	if (!this->color.isValid()) {
		log::log_error("Population type \"" + this->get_identifier() + "\" has no color. A random one will be generated for it.");
		this->color = random::get()->generate_color();
	}

	if (this->culture != nullptr) {
		assert_throw(this->culture->get_population_class_type(this->get_population_class()) == nullptr);

		this->culture->set_population_class_type(this->get_population_class(), this);
	} else if (this->cultural_group != nullptr) {
		assert_throw(this->cultural_group->get_population_class_type(this->get_population_class()) == nullptr);

		this->cultural_group->set_population_class_type(this->get_population_class(), this);
	} else {
		this->population_class->set_default_population_type(this);
	}

	named_data_entry::initialize();
}

void population_type::check() const
{
	assert_throw(this->get_color().isValid());
	assert_throw(this->get_icon() != nullptr);
	assert_throw(this->get_small_icon() != nullptr);

	if (this->get_country_modifier() != nullptr) {
		if (this->get_max_modifier_multiplier() == 0) {
			throw std::runtime_error(std::format("Population type \"{}\" has a country modifier, but has no maximum modifier multiplier.", this->get_identifier()));
		}
	}

	for (const auto &[commodity, demand] : this->get_commodity_demands()) {
		if (!commodity->is_tradeable()) {
			throw std::runtime_error(std::format("Population type \"{}\" demands a non-tradeable commodity (\"{}\").", this->get_identifier(), commodity->get_identifier()));
		}
	}
}

QString population_type::get_country_modifier_string(const metternich::country *country) const
{
	if (this->get_output_commodity() == nullptr && this->get_country_modifier() == nullptr) {
		return QString();
	}

	const country_game_data *country_game_data = country->get_game_data();
	const int population_type_count = country_game_data->get_population()->get_type_count(this);

	std::string str;

	if (this->get_output_commodity() != nullptr) {
		str += std::format("{}: {}", this->get_output_commodity()->get_name(), string::colored(number::to_signed_string(population_type_count * this->get_output_value()), defines::get()->get_green_text_color()));
	}

	if (this->get_country_modifier() != nullptr) {
		const std::string modifier_str = this->get_country_modifier()->get_string(country, centesimal_int::min(population_type_count * country_game_data->get_population_type_modifier_multiplier(this), this->get_max_modifier_multiplier()));
		if (!modifier_str.empty()) {
			if (!str.empty()) {
				str += "\n";
			}

			str += modifier_str;
		}
	}

	return QString::fromStdString(str);
}

}
