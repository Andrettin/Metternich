#pragma once

#include "util/qunique_ptr.h"

namespace metternich {

class country;
class expense_transaction;
class income_transaction;
enum class expense_transaction_type;
enum class income_transaction_type;

class country_turn_data final : public QObject
{
	Q_OBJECT

public:
	explicit country_turn_data(metternich::country *country);
	~country_turn_data();

	int get_total_income() const
	{
		return this->total_income;
	}

	int get_total_expense() const
	{
		return this->total_expense;
	}

	void add_income_transaction(const income_transaction_type transaction_type, const commodity *commodity, const int amount);
	void add_expense_transaction(const expense_transaction_type transaction_type, const commodity *commodity, const int amount);

	std::map<income_transaction_type, int> get_income_transaction_type_percentages() const;

private:
	metternich::country *country = nullptr;
	int total_income = 0;
	int total_expense = 0;
	std::vector<qunique_ptr<income_transaction>> income_transactions;
	std::vector<qunique_ptr<expense_transaction>> expense_transactions;
};

}
