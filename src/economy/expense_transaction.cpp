#include "metternich.h"

#include "economy/expense_transaction.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/expense_transaction_type.h"

namespace metternich {

QString expense_transaction::get_name() const
{
	switch (this->get_type()) {
		case expense_transaction_type::military_upkeep:
			return "Military Upkeep";
		case expense_transaction_type::tax:
			return "Tax";
		default:
			return transaction::get_name();
	}
}

const icon *expense_transaction::get_icon() const
{
	switch (this->get_type()) {
		case expense_transaction_type::military_upkeep:
			return defines::get()->get_military_upkeep_icon();
		case expense_transaction_type::tax:
			return defines::get()->get_tariff_icon();
		default:
			return transaction::get_icon();
	}
}

QString expense_transaction::get_description() const
{
	std::string str;

	switch (this->get_type()) {
		case expense_transaction_type::purchase:
			str = std::format("Bought {} {} from {} for ${}", this->get_commodity_quantity(), this->get_commodity()->get_name(), this->get_country()->get_game_data()->get_name(), this->get_amount());
			break;
		case expense_transaction_type::military_upkeep:
			str = std::format("Paid ${} in military upkeep", this->get_amount());
			break;
		case expense_transaction_type::tax:
			str = std::format("Paid ${} in taxes to {}", this->get_amount(), this->get_country()->get_game_data()->get_name());
			break;
	}

	return QString::fromStdString(str);
}

}
