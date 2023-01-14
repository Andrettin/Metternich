#include "script/modifier_effect/modifier_effect.h"

#include "character/attribute.h"
#include "character/character.h"
#include "database/gsml_property.h"
#include "script/modifier_effect/attribute_modifier_effect.h"

namespace metternich {

template <typename scope_type>
std::unique_ptr<modifier_effect<scope_type>> modifier_effect<scope_type>::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();

	throw std::runtime_error("Invalid modifier effect: \"" + key + "\".");
}

template class modifier_effect<const character>;

}
