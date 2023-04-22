#include "metternich.h"

#include "country/ideology.h"

#include "script/condition/and_condition.h"
#include "script/factor.h"
#include "util/assert_util.h"
#include "util/log_util.h"
#include "util/random.h"

namespace metternich {

ideology::ideology(const std::string &identifier) : named_data_entry(identifier)
{
}

ideology::~ideology()
{
}

void ideology::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "conditions") {
		auto conditions = std::make_unique<and_condition<population_unit>>();
		database::process_gsml_data(conditions, scope);
		this->conditions = std::move(conditions);
	} else if (tag == "weight_factor") {
		auto factor = std::make_unique<metternich::factor<population_unit>>();
		database::process_gsml_data(factor, scope);
		this->weight_factor = std::move(factor);
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

void ideology::initialize()
{
	if (!this->color.isValid()) {
		log::log_error("Ideology \"" + this->get_identifier() + "\" has no color. A random one will be generated for it.");
		this->color = random::get()->generate_color();
	}

	named_data_entry::initialize();
}

void ideology::check() const
{
	assert_throw(this->get_color().isValid());

	if (this->get_conditions() != nullptr) {
		this->get_conditions()->check_validity();
	}

	if (this->get_weight_factor() == nullptr) {
		throw std::runtime_error("Ideology \"" + this->get_identifier() + "\" does not have a weight factor.");
	}
}

}
