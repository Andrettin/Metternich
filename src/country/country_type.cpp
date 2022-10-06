#include "metternich.h"

#include "country/country_type.h"

namespace archimedes {

template class enum_converter<metternich::country_type>;

template <>
const std::string enum_converter<metternich::country_type>::property_class_identifier = "metternich::country_type";

template <>
const std::map<std::string, metternich::country_type> enum_converter<metternich::country_type>::string_to_enum_map = {
	{ "great_power", metternich::country_type::great_power },
	{ "minor_nation", metternich::country_type::minor_nation },
	{ "tribe", metternich::country_type::tribe }
};

template <>
const bool enum_converter<metternich::country_type>::initialized = enum_converter::initialize();

}
