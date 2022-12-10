#include "metternich.h"

#include "script/context.h"

#include "country/country.h"
#include "database/gsml_data.h"
#include "map/province.h"
#include "map/province_game_data.h"
#include "population/population_unit.h"

namespace metternich {

template <bool read_only>
void context_base<read_only>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "source_country") {
		this->source_country = country::get(value);
	} else if (key == "current_country") {
		this->current_country = country::get(value);
	} else {
		throw std::runtime_error("Invalid context property: \"" + key + "\".");
	}
}

template <bool read_only>
void context_base<read_only>::process_gsml_scope(const gsml_data &scope)
{
	throw std::runtime_error("Invalid context scope: \"" + scope.get_tag() + "\".");
}

template <bool read_only>
gsml_data context_base<read_only>::to_gsml_data(const std::string &tag) const
{
	gsml_data data(tag);

	if (this->source_country != nullptr) {
		data.add_property("source_country", this->source_country->get_identifier());
	}

	if (this->current_country != nullptr) {
		data.add_property("current_country", this->current_country->get_identifier());
	}

	return data;
}

context context::from_scope(const population_unit *population_unit)
{
	context ctx;
	ctx.current_country = population_unit->get_province()->get_game_data()->get_owner();
	return ctx;
}

context context::from_scope(const province *province)
{
	context ctx;
	ctx.current_country = province->get_game_data()->get_owner();
	return ctx;
}

read_only_context read_only_context::from_scope(const population_unit *population_unit)
{
	read_only_context ctx;
	ctx.current_country = population_unit->get_province()->get_game_data()->get_owner();
	return ctx;
}

read_only_context read_only_context::from_scope(const province *province)
{
	read_only_context ctx;
	ctx.current_country = province->get_game_data()->get_owner();
	return ctx;
}

template struct context_base<false>;
template struct context_base<true>;

}
