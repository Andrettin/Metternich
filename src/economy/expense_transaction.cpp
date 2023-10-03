#include "metternich.h"

#include "economy/expense_transaction.h"

#include "economy/commodity.h"
#include "economy/expense_transaction_type.h"

namespace metternich {

QString expense_transaction::get_description() const
{
	std::string str;

	switch (this->get_type()) {
		case expense_transaction_type::purchase:
			str = std::format("Bought {} {} for ${}", this->get_commodity_quantity(), this->get_commodity()->get_name(), this->get_amount());
			break;
		case expense_transaction_type::military_upkeep:
			str = std::format("Paid ${} in military upkeep", this->get_amount());
			break;
	}

	return QString::fromStdString(str);
}

}
