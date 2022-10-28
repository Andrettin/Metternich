#include "metternich.h"

#include "unit/historical_civilian_unit.h"

#include "unit/historical_civilian_unit_history.h"
#include "util/assert_util.h"

namespace metternich {

historical_civilian_unit::historical_civilian_unit(const std::string &identifier) : named_data_entry(identifier)
{
}

historical_civilian_unit::~historical_civilian_unit()
{
}

void historical_civilian_unit::check() const
{
	assert_throw(this->get_type() != nullptr);
}

data_entry_history *historical_civilian_unit::get_history_base()
{
	return this->history.get();
}

void historical_civilian_unit::reset_history()
{
	this->history = make_qunique<historical_civilian_unit_history>();
}

}
