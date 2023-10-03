#include "metternich.h"

#include "economy/income_transaction.h"

#include "economy/commodity.h"
#include "economy/income_transaction_type.h"

namespace metternich {

QString income_transaction::get_description() const
{
	std::string str;

	switch (this->get_type()) {
		case income_transaction_type::sale:
			str = std::format("Sold {} {} for ${}", this->get_commodity_quantity(), this->get_commodity()->get_name(), this->get_amount());
			break;
		case income_transaction_type::liquidated_riches:
			str = std::format("Converted {} {} into ${}", this->get_commodity_quantity(), this->get_commodity()->get_name(), this->get_amount());
			break;
	}

	return QString::fromStdString(str);
}

}
