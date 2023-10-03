#include "metternich.h"

#include "country/country.h"

#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/country_turn_data.h"
#include "country/country_type.h"
#include "database/defines.h"
#include "map/province.h"
#include "map/site.h"
#include "time/era.h"
#include "util/assert_util.h"
#include "util/log_util.h"

namespace metternich {

country::country(const std::string &identifier)
	: named_data_entry(identifier), type(country_type::minor_nation)
{
	this->reset_game_data();
}

country::~country()
{
}

void country::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "eras") {
		for (const std::string &value : values) {
			this->eras.push_back(era::get(value));
		}
	} else if (tag == "core_provinces") {
		for (const std::string &value : values) {
			this->core_provinces.push_back(province::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void country::initialize()
{
	for (province *province : this->get_core_provinces()) {
		province->add_core_country(this);
	}

	named_data_entry::initialize();
}

void country::check() const
{
	if (this->get_culture() == nullptr) {
		throw std::runtime_error(std::format("Country \"{}\" has no culture.", this->get_identifier()));
	}

	if (this->get_default_religion() == nullptr) {
		throw std::runtime_error(std::format("Country \"{}\" has no default religion.", this->get_identifier()));
	}

	if (this->get_default_capital() == nullptr) {
		throw std::runtime_error(std::format("Country \"{}\" has no default capital.", this->get_identifier()));
	}

	if (!this->get_default_capital()->is_settlement()) {
		throw std::runtime_error(std::format("The default capital for country \"{}\" (\"{}\") is not a settlement.", this->get_identifier(), this->get_default_capital()->get_identifier()));
	}

	assert_throw(this->get_color().isValid());
}

data_entry_history *country::get_history_base()
{
	return this->history.get();
}

void country::reset_history()
{
	this->history = make_qunique<country_history>(this);
}

void country::reset_game_data()
{
	this->game_data = make_qunique<country_game_data>(this);
	this->get_game_data()->initialize_building_slots();

	this->reset_turn_data();
}

void country::reset_turn_data()
{
	this->turn_data = make_qunique<country_turn_data>(this);
}

bool country::is_great_power() const
{
	return this->get_type() == country_type::great_power;
}

bool country::is_tribe() const
{
	return this->get_type() == country_type::tribe;
}

const QColor &country::get_color() const
{
	if (this->get_type() != country_type::great_power) {
		return defines::get()->get_minor_nation_color();
	}

	return this->color;
}

const population_class *country::get_default_population_class() const
{
	if (this->is_tribe()) {
		return defines::get()->get_default_tribal_population_class();
	} else {
		return defines::get()->get_default_population_class();
	}
}

bool country::can_declare_war() const
{
	return this->get_type() == country_type::great_power;
}

}
