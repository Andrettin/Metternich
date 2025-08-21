#include "metternich.h"

#include "economy/commodity_unit.h"

#include "util/assert_util.h"

namespace metternich {

commodity_unit::commodity_unit(const std::string &identifier) : named_data_entry(identifier)
{
}

void commodity_unit::check() const
{
	assert_throw(this->get_icon() != nullptr);
}

}
