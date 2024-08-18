#include "metternich.h"

#include "unit/transporter_stat.h"

namespace archimedes {

template class enum_converter<metternich::transporter_stat>;

template <>
const std::string enum_converter<metternich::transporter_stat>::property_class_identifier = "metternich::transporter_stat";

template <>
const std::map<std::string, metternich::transporter_stat> enum_converter<metternich::transporter_stat>::string_to_enum_map = {
	{ "defense", metternich::transporter_stat::defense },
	{ "resistance", metternich::transporter_stat::resistance },
	{ "movement", metternich::transporter_stat::movement }
};

template <>
const bool enum_converter<metternich::transporter_stat>::initialized = enum_converter::initialize();

}
