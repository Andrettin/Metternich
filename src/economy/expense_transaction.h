#pragma once

#include "economy/transaction.h"

namespace metternich {

enum class expense_transaction_type;

class expense_transaction final : public transaction
{
	Q_OBJECT

public:
	explicit expense_transaction(const expense_transaction_type type, const metternich::commodity *commodity, const int amount)
		: transaction(commodity, amount), type(type)
	{
	}

	expense_transaction_type get_type() const
	{
		return this->type;
	}

private:
	expense_transaction_type type{};
};

}
