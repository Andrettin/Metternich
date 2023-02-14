#include "metternich.h"

#include "script/context.h"

#include "character/character.h"
#include "country/country.h"
#include "database/gsml_data.h"
#include "map/province.h"
#include "map/site.h"
#include "population/population_unit.h"
#include "util/assert_util.h"

namespace metternich {

template <bool read_only>
void context_base<read_only>::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "root_character") {
		this->root_scope = character::get(value);
	} else if (key == "root_country") {
		this->root_scope = country::get(value);
	} else if (key == "root_province") {
		this->root_scope = province::get(value);
	} else if (key == "root_site") {
		this->root_scope = site::get(value);
	} else if (key == "source_character") {
		this->source_scope = character::get(value);
	} else if (key == "source_country") {
		this->source_scope = country::get(value);
	} else if (key == "source_province") {
		this->source_scope = province::get(value);
	} else if (key == "source_site") {
		this->source_scope = site::get(value);
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

	if (std::holds_alternative<const character *>(this->root_scope)) {
		data.add_property("root_character", std::get<const character *>(this->root_scope)->get_identifier());
	} else if (std::holds_alternative<const country *>(this->root_scope)) {
		data.add_property("root_country", std::get<const country *>(this->root_scope)->get_identifier());
	} else if (std::holds_alternative<const province *>(this->root_scope)) {
		data.add_property("root_province", std::get<const province *>(this->root_scope)->get_identifier());
	} else if (std::holds_alternative<const site *>(this->root_scope)) {
		data.add_property("root_site", std::get<const site *>(this->root_scope)->get_identifier());
	} else {
		assert_throw(false);
	}

	if (std::holds_alternative<const character *>(this->source_scope)) {
		data.add_property("source_character", std::get<const character *>(this->source_scope)->get_identifier());
	} else if (std::holds_alternative<const country *>(this->source_scope)) {
		data.add_property("source_country", std::get<const country *>(this->source_scope)->get_identifier());
	} else if (std::holds_alternative<const province *>(this->source_scope)) {
		data.add_property("source_province", std::get<const province *>(this->source_scope)->get_identifier());
	} else if (std::holds_alternative<const site *>(this->source_scope)) {
		data.add_property("source_site", std::get<const site *>(this->source_scope)->get_identifier());
	} else {
		assert_throw(false);
	}

	return data;
}

template struct context_base<false>;
template struct context_base<true>;

}
