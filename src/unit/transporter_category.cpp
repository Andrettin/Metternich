#include "metternich.h"

#include "unit/transporter_category.h"

namespace archimedes {

template class enum_converter<metternich::transporter_category>;

template <>
const std::string enum_converter<metternich::transporter_category>::property_class_identifier = "metternich::transporter_category";

template <>
const std::map<std::string, metternich::transporter_category> enum_converter<metternich::transporter_category>::string_to_enum_map = {
	{ "land_transporter", metternich::transporter_category::land_transporter },
	{ "small_merchant_ship", metternich::transporter_category::small_merchant_ship },
	{ "large_merchant_ship", metternich::transporter_category::large_merchant_ship }
};

template <>
const bool enum_converter<metternich::transporter_category>::initialized = enum_converter::initialize();

}
