#pragma once

#include "economy/transaction.h"

namespace metternich {

enum class income_transaction_type;

class income_transaction final : public transaction
{
	Q_OBJECT

public:
	explicit income_transaction(const income_transaction_type type, const metternich::commodity *commodity, const int amount)
		: transaction(commodity, amount), type(type)
	{
	}

	income_transaction_type get_type() const
	{
		return this->type;
	}

private:
	income_transaction_type type{};
};

}
