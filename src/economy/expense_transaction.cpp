#include "metternich.h"

#include "economy/expense_transaction.h"

#include "database/defines.h"
#include "domain/domain.h"
#include "domain/domain_game_data.h"
#include "economy/commodity.h"
#include "economy/expense_transaction_type.h"
#include "util/number_util.h"

namespace metternich {

QString expense_transaction::get_name() const
{
	switch (this->get_type()) {
		case expense_transaction_type::domain_maintenance:
			return "Domain Maintenance";
		case expense_transaction_type::military_maintenance:
			return "Military Maintenance";
		case expense_transaction_type::tribute:
			return "Tribute";
		default:
			return transaction::get_name();
	}
}

const icon *expense_transaction::get_icon() const
{
	switch (this->get_type()) {
		case expense_transaction_type::domain_maintenance:
		case expense_transaction_type::military_maintenance:
			return defines::get()->get_wealth_commodity()->get_icon();
		case expense_transaction_type::tribute:
			return defines::get()->get_tariff_icon();
		default:
			return transaction::get_icon();
	}
}

QString expense_transaction::get_description() const
{
	std::string str;

	const std::string amount_str = defines::get()->get_wealth_commodity()->value_to_string(this->get_amount());

	switch (this->get_type()) {
		case expense_transaction_type::purchase:
			str = std::format("Bought {} {} from {} for {}", number::to_formatted_string(this->get_object_quantity()), this->get_object_name(), this->get_country()->get_game_data()->get_name(), amount_str);
			break;
		case expense_transaction_type::domain_maintenance:
			str = std::format("Paid {} in domain maintenance", amount_str);
			break;
		case expense_transaction_type::military_maintenance:
		{
			if (std::holds_alternative<const metternich::commodity *>(this->get_object())) {
				const commodity *commodity = std::get<const metternich::commodity *>(this->get_object());
				str = std::format("Spent {} {} in military maintenance", commodity->value_to_string(this->get_object_quantity()), commodity->get_name());
			} else {
				str = std::format("Paid {} in military maintenance", amount_str);
			}
			break;
		}
		case expense_transaction_type::tribute:
			str = std::format("Paid {} in tribute to {}", amount_str, this->get_country()->get_game_data()->get_name());
			break;
	}

	return QString::fromStdString(str);
}

}
