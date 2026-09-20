#include "metternich.h"

#include "character/level_bonus_table.h"

#include "util/assert_util.h"

namespace metternich {

void level_bonus_table::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	assert_throw(property.get_operator() == gsml_operator::assignment);

	const int level = std::stoi(key);

	if (value.find("d") != std::string::npos) {
		this->bonus_per_level[level] = dice(value);
	} else {
		this->bonus_per_level[level] = std::stoi(value);
	}
}

void level_bonus_table::check() const
{
	if (this->bonus_per_level.empty()) {
		throw std::runtime_error(std::format("Level bonus table \"{}\" has no data.", this->get_identifier()));
	}
}

}
