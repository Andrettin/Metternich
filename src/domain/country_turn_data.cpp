#include "metternich.h"

#include "domain/country_turn_data.h"

#include "economy/expense_transaction.h"
#include "economy/income_transaction.h"
#include "economy/income_transaction_type.h"
#include "util/container_util.h"

namespace metternich {

country_turn_data::country_turn_data(const metternich::domain *domain) : domain(domain)
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

void country_turn_data::add_income_transaction(const income_transaction_type transaction_type, const int amount, const transaction_object_variant &object, const int object_quantity, const metternich::domain *other_domain)
{
	this->total_income += amount;

	for (const qunique_ptr<income_transaction> &transaction : this->income_transactions) {
		if (transaction->get_type() != transaction_type) {
			continue;
		}

		if (transaction->get_object() != object) {
			continue;
		}

		transaction->change_amount(amount);
		transaction->change_object_quantity(object_quantity);
		return;
	}

	auto transaction = make_qunique<metternich::income_transaction>(transaction_type, amount, object, object_quantity, other_domain);
	this->income_transactions.push_back(std::move(transaction));
}

void country_turn_data::add_expense_transaction(const expense_transaction_type transaction_type, const int amount, const transaction_object_variant &object, const int object_quantity, const metternich::domain *other_domain)
{
	this->total_expense += amount;

	for (const qunique_ptr<expense_transaction> &transaction : this->expense_transactions) {
		if (transaction->get_type() != transaction_type) {
			continue;
		}

		if (transaction->get_object() != object) {
			continue;
		}

		transaction->change_amount(amount);
		transaction->change_object_quantity(object_quantity);

		if (transaction->get_amount() == 0 && transaction->get_object_quantity() == 0) {
			std::erase(this->expense_transactions, transaction);
		}

		return;
	}

	auto transaction = make_qunique<metternich::expense_transaction>(transaction_type, amount, object, object_quantity, other_domain);
	this->expense_transactions.push_back(std::move(transaction));
}

}
