#include "metternich.h"

#include "economy/income_transaction.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/income_transaction_type.h"
#include "util/number_util.h"

namespace metternich {

QString income_transaction::get_name() const
{
	switch (this->get_type()) {
		case income_transaction_type::tariff:
			return "Tariff";
		case income_transaction_type::treasure_fleet:
			return "Treasure Fleet";
		default:
			return transaction::get_name();
	}
}

const icon *income_transaction::get_icon() const
{
	switch (this->get_type()) {
		case income_transaction_type::tariff:
			return defines::get()->get_tariff_icon();
		case income_transaction_type::treasure_fleet:
			return defines::get()->get_treasure_fleet_icon();
		default:
			return transaction::get_icon();
	}
}

QString income_transaction::get_description() const
{
	std::string str;

	const std::string amount_str = number::to_formatted_string(this->get_amount());

	switch (this->get_type()) {
		case income_transaction_type::sale:
			str = std::format("Sold {} {} to {} for ${}", number::to_formatted_string(this->get_object_quantity()), std::get<const commodity *>(this->get_object())->get_name(), this->get_country() ? this->get_country()->get_game_data()->get_name() : "the domestic market", amount_str);
			break;
		case income_transaction_type::liquidated_riches:
			str = std::format("Converted {} {} into ${}\n+{}% Inflation", number::to_formatted_string(this->get_object_quantity()), std::get<const commodity *>(this->get_object())->get_name(), amount_str, this->inflation_change.to_string());
			break;
		case income_transaction_type::tariff:
			str = std::format("Received ${} in tariffs from {}", amount_str, this->get_country()->get_game_data()->get_name());
			break;
		case income_transaction_type::treasure_fleet:
			str = std::format("Received treasure fleet worth ${} from {}\n+{}% Inflation", amount_str, this->get_country()->get_game_data()->get_name(), this->inflation_change.to_string());
			break;
	}

	return QString::fromStdString(str);
}

}
