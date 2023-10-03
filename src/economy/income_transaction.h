#pragma once

#include "economy/transaction.h"
#include "util/fractional_int.h"

namespace metternich {

enum class income_transaction_type;

class income_transaction final : public transaction
{
	Q_OBJECT

public:
	explicit income_transaction(const income_transaction_type type, const int amount, const metternich::commodity *commodity, const int commodity_quantity)
		: transaction(amount, commodity, commodity_quantity), type(type)
	{
	}

	income_transaction_type get_type() const
	{
		return this->type;
	}

	void set_inflation_change(const centesimal_int &inflation_change)
	{
		this->inflation_change = inflation_change;
	}

	virtual QString get_description() const override;

private:
	income_transaction_type type{};
	centesimal_int inflation_change;
};

}
