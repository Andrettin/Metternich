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

	const std::string amount_str = number::to_formatted_string(this->get_amount());

	switch (this->get_type()) {
		case expense_transaction_type::purchase:
			str = std::format("Bought {} {} from {} for ${}", number::to_formatted_string(this->get_object_quantity()), this->get_object_name(), this->get_country()->get_game_data()->get_name(), amount_str);
			break;
		case expense_transaction_type::population_upkeep:
			str = std::format("Supported {} {} for ${}", number::to_formatted_string(this->get_object_quantity()), this->get_object_name(), amount_str);
			break;
		case expense_transaction_type::military_upkeep:
			str = std::format("Paid ${} in military upkeep", amount_str);
			break;
		case expense_transaction_type::tax:
			str = std::format("Paid ${} in taxes to {}", amount_str, this->get_country()->get_game_data()->get_name());
			break;
	}

	return QString::fromStdString(str);
}

}
