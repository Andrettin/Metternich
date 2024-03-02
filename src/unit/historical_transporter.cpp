#include "metternich.h"

#include "unit/historical_transporter.h"

#include "unit/historical_transporter_history.h"
#include "util/assert_util.h"

namespace metternich {

historical_transporter::historical_transporter(const std::string &identifier) : named_data_entry(identifier)
{
}

historical_transporter::~historical_transporter()
{
}

void historical_transporter::check() const
{
	assert_throw(this->get_type() != nullptr);
}

data_entry_history *historical_transporter::get_history_base()
{
	return this->history.get();
}

void historical_transporter::reset_history()
{
	this->history = make_qunique<historical_transporter_history>();
}

}
