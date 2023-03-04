#include "metternich.h"

#include "economy/commodity.h"

#include "economy/food_type.h"
#include "util/assert_util.h"

namespace metternich {

commodity::commodity(const std::string &identifier)
	: named_data_entry(identifier), food_type(food_type::none)
{
}

void commodity::check() const
{
	assert_throw(this->get_icon() != nullptr);
	assert_throw(this->get_wealth_value() >= 0);
}

bool commodity::is_food() const
{
	return this->get_food_type() != food_type::none;
}

}
