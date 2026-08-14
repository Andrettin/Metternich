#include "metternich.h"

#include "database/population_defines.h"

#include "script/factor.h"

namespace metternich {

population_defines::population_defines()
{
}

population_defines::~population_defines()
{
}

void population_defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "promotion_chance") {
		this->promotion_chance = std::make_unique<factor<population_unit>>();
		this->promotion_chance->process_gsml_data(scope);
	} else if (tag == "demotion_chance") {
		this->demotion_chance = std::make_unique<factor<population_unit>>();
		this->demotion_chance->process_gsml_data(scope);
	} else {
		defines_base::process_gsml_scope(scope);
	}
}

}
