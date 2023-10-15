#include "metternich.h"

#include "economy/income_transaction.h"

#include "country/country.h"
#include "country/country_game_data.h"
#include "database/defines.h"
#include "economy/commodity.h"
#include "economy/income_transaction_type.h"

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

	switch (this->get_type()) {
		case income_transaction_type::sale:
			str = std::format("Sold {} {} to {} for ${}", this->get_commodity_quantity(), this->get_commodity()->get_name(), this->get_country()->get_game_data()->get_name(), this->get_amount());
			break;
		case income_transaction_type::liquidated_riches:
			str = std::format("Converted {} {} into ${}\n+{}% Inflation", this->get_commodity_quantity(), this->get_commodity()->get_name(), this->get_amount(), this->inflation_change.to_string());
			break;
		case income_transaction_type::tariff:
			str = std::format("Received ${} in tariffs from {}", this->get_amount(), this->get_country()->get_game_data()->get_name());
			break;
		case income_transaction_type::treasure_fleet:
			str = std::format("Received treasure fleet worth ${} from {}\n+{}% Inflation", this->get_amount(), this->get_country()->get_game_data()->get_name(), this->inflation_change.to_string());
			break;
	}

	return QString::fromStdString(str);
}

}
