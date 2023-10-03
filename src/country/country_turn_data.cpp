#include "metternich.h"

#include "country/country_turn_data.h"

#include "economy/expense_transaction.h"
#include "economy/income_transaction.h"

namespace metternich {

country_turn_data::country_turn_data(metternich::country *country) : country(country)
{
}

country_turn_data::~country_turn_data()
{
}

void country_turn_data::add_income_transaction(const income_transaction_type transaction_type, const commodity *commodity, const int amount)
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
		return;
	}

	auto transaction = make_qunique<metternich::income_transaction>(transaction_type, commodity, amount);
	this->income_transactions.push_back(std::move(transaction));
}

void country_turn_data::add_expense_transaction(const expense_transaction_type transaction_type, const commodity *commodity, const int amount)
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
		return;
	}

	auto transaction = make_qunique<metternich::expense_transaction>(transaction_type, commodity, amount);
	this->expense_transactions.push_back(std::move(transaction));
}

std::map<income_transaction_type, int> country_turn_data::get_income_transaction_type_percentages() const
{
	std::map<income_transaction_type, int> amounts;

	for (const qunique_ptr<income_transaction> &transaction : this->income_transactions) {
		amounts[transaction->get_type()] += transaction->get_amount();
	}

	std::map<income_transaction_type, int> percentages;

	for (const auto &[type, amount] : amounts) {
		percentages[type] = amount * 100 / this->get_total_income();
	}

	return percentages;
}

}
