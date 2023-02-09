#include "script/opinion_modifier.h"

#include "database/defines.h"
#include "database/gsml_property.h"

namespace metternich {

void opinion_modifier::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "days") {
		this->duration = std::max(1, defines::get()->days_to_turns(std::stoi(value)).to_int());
	} else if (key == "months") {
		this->duration = std::max(1, defines::get()->months_to_turns(std::stoi(value)).to_int());
	} else if (key == "years") {
		this->duration = std::max(1, defines::get()->years_to_turns(std::stoi(value)).to_int());
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

}
