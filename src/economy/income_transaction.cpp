#include "metternich.h"

#include "economy/income_transaction.h"

#include "country/country.h"
#include "economy/commodity.h"
#include "economy/income_transaction_type.h"

namespace metternich {

QString income_transaction::get_description() const
{
	std::string str;

	switch (this->get_type()) {
		case income_transaction_type::sale:
			str = std::format("Sold {} {} to {} for ${}", this->get_commodity_quantity(), this->get_commodity()->get_name(), this->get_country()->get_name(), this->get_amount());
			break;
		case income_transaction_type::liquidated_riches:
			str = std::format("Converted {} {} into ${}\n+{}% Inflation", this->get_commodity_quantity(), this->get_commodity()->get_name(), this->get_amount(), this->inflation_change.to_string());
			break;
	}

	return QString::fromStdString(str);
}

}
