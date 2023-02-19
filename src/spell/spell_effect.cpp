#include "spell/spell_effect.h"

#include "database/database.h"
#include "database/gsml_data.h"
#include "database/gsml_operator.h"
#include "database/gsml_property.h"
#include "spell/damage_spell_effect.h"
#include "spell/healing_spell_effect.h"
#include "util/assert_util.h"

namespace metternich {

qunique_ptr<spell_effect> spell_effect::from_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "damage") {
		return make_qunique<damage_spell_effect>(value);
	} else if (key == "healing") {
		return make_qunique<healing_spell_effect>(value);
	}

	throw std::runtime_error("Invalid spell effect: \"" + key + "\".");
}

qunique_ptr<spell_effect> spell_effect::from_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	qunique_ptr<spell_effect> effect;

	assert_throw(scope.get_operator() == gsml_operator::assignment);

	if (tag == "damage") {
		effect = make_qunique<damage_spell_effect>();
	} else if (tag == "healing") {
		effect = make_qunique<healing_spell_effect>();
	}

	if (effect == nullptr) {
		throw std::runtime_error("Invalid spell effect: \"" + tag + "\".");
	}

	database::process_gsml_data(effect, scope);

	return effect;
}

}
