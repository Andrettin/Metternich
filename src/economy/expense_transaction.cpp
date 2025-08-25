#include "metternich.h"

#include "economy/expense_transaction.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/expense_transaction_type.h"
#include "util/number_util.h"

namespace metternich {

QString expense_transaction::get_name() const
{
	switch (this->get_type()) {
		case expense_transaction_type::military_maintenance:
			return "Military Maintenance";
		case expense_transaction_type::tax:
			return "Tax";
		default:
			return transaction::get_name();
	}
}

const icon *expense_transaction::get_icon() const
{
	switch (this->get_type()) {
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
		case expense_transaction_type::military_maintenance:
		{
			const commodity *commodity = std::get<const metternich::commodity *>(this->get_object());
			if (commodity == defines::get()->get_wealth_commodity()) {
				str = std::format("Paid {} in military maintenance", commodity->value_to_string(this->get_object_quantity()));
			} else {
				str = std::format("Spent {} {} in military maintenance", commodity->value_to_string(this->get_object_quantity()), commodity->get_name());
			}
			break;
		}
		case expense_transaction_type::tax:
			str = std::format("Paid ${} in taxes to {}", amount_str, this->get_country()->get_game_data()->get_name());
			break;
	}

	return QString::fromStdString(str);
}

}
