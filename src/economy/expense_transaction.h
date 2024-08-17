#pragma once

#include "economy/transaction.h"

namespace metternich {

enum class expense_transaction_type;

class expense_transaction final : public transaction
{
	Q_OBJECT

public:
	explicit expense_transaction(const expense_transaction_type type, const int amount, const object_variant &object, const int object_quantity, const metternich::country *country)
		: transaction(amount, object, object_quantity, country), type(type)
	{
	}

	expense_transaction_type get_type() const
	{
		return this->type;
	}

	virtual QString get_name() const override;
	virtual const icon *get_icon() const override;
	virtual QString get_description() const override;
private:
	expense_transaction_type type{};
};

}
