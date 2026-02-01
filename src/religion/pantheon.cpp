#include "metternich.h"

#include "religion/pantheon.h"

#include "religion/divine_rank.h"

#include <magic_enum/magic_enum.hpp>

namespace metternich {

void pantheon::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "divine_rank_names") {
		scope.for_each_property([&](const gsml_property &property) {
			const std::string &key = property.get_key();
			const std::string &value = property.get_value();

			this->divine_rank_names[magic_enum::enum_cast<divine_rank>(key).value()] = value;
		});
	} else {
		data_entry::process_gsml_scope(scope);
	}
}

std::string_view pantheon::get_divine_rank_name(const divine_rank rank) const
{
	const auto find_iterator = this->divine_rank_names.find(rank);

	if (find_iterator != this->divine_rank_names.end()) {
		return find_iterator->second;
	}

	switch (rank) {
		case divine_rank::quasi_deity:
			return "Quasi-Deity";
		case divine_rank::demigod:
			return "Demigod";
		case divine_rank::lesser_deity:
			return "Lesser Deity";
		case divine_rank::intermediate_deity:
			return "Intermediate Deity";
		case divine_rank::greater_deity:
			return "Greater Deity";
		case divine_rank::overdeity:
			return "Overdeity";
		default:
			break;
	}

	throw std::runtime_error(std::format("Invalid divine rank \"{}\".", magic_enum::enum_name(rank)));
}

}
