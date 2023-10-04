#include "metternich.h"

#include "country/country_turn_data.h"

#include "economy/expense_transaction.h"
#include "economy/income_transaction.h"
#include "economy/income_transaction_type.h"
#include "util/container_util.h"

namespace metternich {

country_turn_data::country_turn_data(metternich::country *country) : country(country)
{
}

country_turn_data::~country_turn_data()
{
}

QVariantList country_turn_data::get_income_transactions_qvariant_list() const
{
	return container::to_qvariant_list(this->income_transactions);
}

QVariantList country_turn_data::get_expense_transactions_qvariant_list() const
{
	return container::to_qvariant_list(this->expense_transactions);
}

void country_turn_data::add_income_transaction(const income_transaction_type transaction_type, const int amount, const commodity *commodity, const int commodity_quantity, const metternich::country *other_country)
{
	this->total_income += amount;

	for (const qunique_ptr<income_transaction> &transaction : this->income_transactions) {
		if (transaction->get_type() != transaction_type) {
			continue;
		}

		if (transaction->get_commodity() != commodity) {
			continue;
		}

		transaction->change_amount(amount);
		transaction->change_commodity_quantity(commodity_quantity);
		return;
	}

	auto transaction = make_qunique<metternich::income_transaction>(transaction_type, amount, commodity, commodity_quantity, other_country);
	this->income_transactions.push_back(std::move(transaction));
}

void country_turn_data::add_expense_transaction(const expense_transaction_type transaction_type, const int amount, const commodity *commodity, const int commodity_quantity, const metternich::country *other_country)
{
	this->total_expense += amount;

	for (const qunique_ptr<expense_transaction> &transaction : this->expense_transactions) {
		if (transaction->get_type() != transaction_type) {
			continue;
		}

		if (transaction->get_commodity() != commodity) {
			continue;
		}

		transaction->change_amount(amount);
		transaction->change_commodity_quantity(commodity_quantity);
		return;
	}

	auto transaction = make_qunique<metternich::expense_transaction>(transaction_type, amount, commodity, commodity_quantity, other_country);
	this->expense_transactions.push_back(std::move(transaction));
}

void country_turn_data::calculate_inflation()
{
	if (this->get_total_income() == 0) {
		return;
	}

	std::map<income_transaction_type, int> amounts;

	int liquidated_riches_amount = 0;

	for (const qunique_ptr<income_transaction> &transaction : this->income_transactions) {
		if (transaction->get_type() == income_transaction_type::liquidated_riches) {
			const centesimal_int inflation_change = country_turn_data::base_inflation_change * transaction->get_amount() / this->get_total_income();
			transaction->set_inflation_change(inflation_change);

			liquidated_riches_amount += transaction->get_amount();
		}
	}

	this->total_inflation_change = country_turn_data::base_inflation_change * liquidated_riches_amount / this->get_total_income();
}

}
