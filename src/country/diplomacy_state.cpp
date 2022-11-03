#include "metternich.h"

#include "country/diplomacy_state.h"

namespace archimedes {

template class enum_converter<metternich::diplomacy_state>;

template <>
const std::string enum_converter<metternich::diplomacy_state>::property_class_identifier = "metternich::diplomacy_state";

template <>
const std::map<std::string, metternich::diplomacy_state> enum_converter<metternich::diplomacy_state>::string_to_enum_map = {
	{ "peace", metternich::diplomacy_state::peace },
	{ "alliance", metternich::diplomacy_state::alliance },
	{ "war", metternich::diplomacy_state::war },
	{ "non_aggression_pact", metternich::diplomacy_state::non_aggression_pact },
	{ "vassal", metternich::diplomacy_state::vassal },
	{ "overlord", metternich::diplomacy_state::overlord },
	{ "personal_union_subject", metternich::diplomacy_state::personal_union_subject },
	{ "personal_union_overlord", metternich::diplomacy_state::personal_union_overlord },
	{ "colony", metternich::diplomacy_state::colony },
	{ "colonial_overlord", metternich::diplomacy_state::colonial_overlord }
};

template <>
const bool enum_converter<metternich::diplomacy_state>::initialized = enum_converter::initialize();

}
