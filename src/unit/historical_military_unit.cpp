#include "metternich.h"

#include "unit/historical_military_unit.h"

#include "unit/historical_military_unit_history.h"
#include "util/assert_util.h"

namespace metternich {

historical_military_unit::historical_military_unit(const std::string &identifier) : named_data_entry(identifier)
{
}

historical_military_unit::~historical_military_unit()
{
}

void historical_military_unit::check() const
{
	assert_throw(this->get_type() != nullptr);
}

data_entry_history *historical_military_unit::get_history_base()
{
	return this->history.get();
}

void historical_military_unit::reset_history()
{
	this->history = make_qunique<historical_military_unit_history>();
}

}
