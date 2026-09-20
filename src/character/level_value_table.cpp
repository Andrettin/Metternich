#include "metternich.h"

#include "character/level_value_table.h"

#include "util/assert_util.h"

namespace metternich {

void level_value_table::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	assert_throw(property.get_operator() == gsml_operator::assignment);

	const int level = std::stoi(key);

	if (value.find("d") != std::string::npos) {
		this->value_per_level[level] = dice(value);
	} else {
		this->value_per_level[level] = std::stoi(value);
	}
}

void level_value_table::check() const
{
	if (this->value_per_level.empty()) {
		throw std::runtime_error(std::format("Level value table \"{}\" has no data.", this->get_identifier()));
	}
}

}
