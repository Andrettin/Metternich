#include "metternich.h"

#include "country/country.h"

#include "country/country_game_data.h"
#include "country/country_history.h"
#include "country/country_type.h"
#include "database/defines.h"
#include "map/province.h"
#include "time/era.h"
#include "util/assert_util.h"
#include "util/log_util.h"

namespace metternich {

country::country(const std::string &identifier)
	: named_data_entry(identifier), type(country_type::minor_nation)
{
	this->reset_game_data();
}

void country::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "eras") {
		for (const std::string &value : values) {
			this->eras.push_back(era::get(value));
		}
	} else if (tag == "provinces") {
		for (const std::string &value : values) {
			this->provinces.push_back(province::get(value));
		}
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void country::check() const
{
	if (this->get_culture() == nullptr) {
		log::log_error("Country \"" + this->get_identifier() + "\" has no culture.");
	}

	assert_throw(this->get_capital_province() != nullptr);
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
}

const QColor &country::get_color() const
{
	if (this->get_type() != country_type::great_power) {
		return defines::get()->get_minor_nation_color();
	}

	return this->color;
}

}
