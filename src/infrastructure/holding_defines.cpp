#include "metternich.h"

#include "infrastructure/holding_defines.h"

namespace metternich {

holding_defines::holding_defines()
{
}

holding_defines::~holding_defines()
{
}

void holding_defines::process_gsml_scope(const gsml_data &scope)
{
	const std::string &tag = scope.get_tag();

	if (tag == "consumption_per_holding_level") {
		scope.for_each_property([this](const gsml_property &property) {
			const int holding_level = std::stoi(property.get_key());
			const centesimal_int consumption(property.get_value());

			this->consumption_per_holding_level[holding_level] = consumption;
		});
	} else {
		defines_base::process_gsml_scope(scope);
	}
}

const centesimal_int &holding_defines::get_consumption_for_holding_level(const int holding_level) const
{
	const auto find_iterator = this->consumption_per_holding_level.upper_bound(holding_level);
	assert_throw(find_iterator != this->consumption_per_holding_level.begin());
	return std::prev(find_iterator)->second;
}

}
