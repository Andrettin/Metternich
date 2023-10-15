#include "metternich.h"

#include "economy/transaction.h"

#include "economy/commodity.h"
#include "util/assert_util.h"
#include "util/number_util.h"

namespace metternich {

QString transaction::get_name() const
{
	assert_throw(this->get_commodity() != nullptr);

	return QString::fromStdString(std::format("{} ${}", this->get_commodity()->get_name(), number::to_formatted_string(this->get_amount() / this->get_commodity_quantity())));
}

const icon *transaction::get_icon() const
{
	assert_throw(this->get_commodity() != nullptr);

	return this->get_commodity()->get_icon();
}

}
