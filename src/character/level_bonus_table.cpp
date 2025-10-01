#include "metternich.h"

#include "character/level_bonus_table.h"

namespace metternich {

void level_bonus_table::process_gsml_property(const gsml_property &property)
{
	const std::string &key = property.get_key();
	const std::string &value = property.get_value();

	this->bonus_per_level[std::stoi(key)] = std::stoi(value);
}

void level_bonus_table::check() const
{
	if (this->get_bonus_per_level().empty()) {
		throw std::runtime_error(std::format("Level bonus table \"{}\" has no data.", this->get_identifier()));
	}
}

}
