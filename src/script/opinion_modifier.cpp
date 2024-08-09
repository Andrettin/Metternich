#include "script/opinion_modifier.h"

#include "database/defines.h"
#include "database/gsml_property.h"

namespace metternich {

void opinion_modifier::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	if (key == "days") {
		this->duration_days = std::stoi(value);
	} else if (key == "months") {
		this->duration_days = std::stoi(value) * 30;
	} else if (key == "years") {
		this->duration_days = std::stoi(value) * 365;
	} else {
		named_data_entry::process_gsml_property(property);
	}
}

}
