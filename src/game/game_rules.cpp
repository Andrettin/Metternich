#include "metternich.h"

#include "game/game_rules.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "util/string_conversion_util.h"

namespace metternich {

void game_rules::process_gsml_property(const gsml_property &property)
{
	database::get()->process_gsml_property_for_object(this, property);
}

void game_rules::process_gsml_scope(const gsml_data &scope)
{
	database::get()->process_gsml_scope_for_object(this, scope);
}

gsml_data game_rules::to_gsml_data() const
{
	gsml_data data;

	data.add_property("myths_enabled", string::from_bool(this->are_myths_enabled()));

	return data;
}

}
