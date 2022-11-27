#include "metternich.h"

#include "country/cultural_group_rank.h"

namespace archimedes {

template class enum_converter<metternich::cultural_group_rank>;

template <>
const std::string enum_converter<metternich::cultural_group_rank>::property_class_identifier = "metternich::cultural_group_rank";

template <>
const std::map<std::string, metternich::cultural_group_rank> enum_converter<metternich::cultural_group_rank>::string_to_enum_map = {
	{ "none", metternich::cultural_group_rank::none },
	{ "infragroup", metternich::cultural_group_rank::infragroup },
	{ "subgroup", metternich::cultural_group_rank::subgroup },
	{ "group", metternich::cultural_group_rank::group },
	{ "supergroup", metternich::cultural_group_rank::supergroup }
};

template <>
const bool enum_converter<metternich::cultural_group_rank>::initialized = enum_converter::initialize();

}
