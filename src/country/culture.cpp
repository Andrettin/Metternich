#include "metternich.h"

#include "country/culture.h"

#include "country/cultural_group.h"
#include "infrastructure/building_class.h"
#include "population/population_class.h"
#include "script/condition/and_condition.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"

namespace metternich {

culture::culture(const std::string &identifier) : culture_base(identifier)
{
}

culture::~culture()
{
}

void culture::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();
	const std::vector<std::string> &values = scope.get_values();

	if (tag == "derived_cultures") {
		for (const std::string &value : values) {
			this->derived_cultures.push_back(culture::get(value));
		}
	} else if (tag == "derivation_conditions") {
		auto conditions = std::make_unique<and_condition<population_unit>>();
		database::process_gsml_data(conditions, scope);
		this->derivation_conditions = std::move(conditions);
	} else {
		culture_base::process_gsml_scope(scope);
	}
}

void culture::initialize()
{
	if (!this->color.isValid()) {
		log::log_error("Culture \"" + this->get_identifier() + "\" has no color. A random one will be generated for it.");
		this->color = random::get()->generate_color();
	}

	culture_base::initialize();
}

void culture::check() const
{
	assert_throw(this->get_color().isValid());

	if (this->get_default_phenotype() == nullptr) {
		throw std::runtime_error("Culture \"" + this->get_identifier() + "\" has no default phenotype.");
	}

	culture_base::check();
}

}
