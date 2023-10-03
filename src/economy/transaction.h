#pragma once

Q_MOC_INCLUDE("economy/commodity.h")

namespace metternich {

class commodity;

class transaction : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::commodity* commodity READ get_commodity CONSTANT)
	Q_PROPERTY(int amount READ get_amount CONSTANT)
	Q_PROPERTY(int commodity_quantity READ get_commodity_quantity CONSTANT)
	Q_PROPERTY(QString description READ get_description CONSTANT)

public:
	explicit transaction(const int amount, const metternich::commodity *commodity, const int commodity_quantity)
		: commodity(commodity), amount(amount), commodity_quantity(commodity_quantity)
	{
	}

	const metternich::commodity *get_commodity() const
	{
		return this->commodity;
	}

	int get_amount() const
	{
		return this->amount;
	}

	void change_amount(const int change)
	{
		if (change == 0) {
			return;
		}

		this->amount += change;
	}

	int get_commodity_quantity() const
	{
		return this->commodity_quantity;
	}

	void change_commodity_quantity(const int change)
	{
		if (change == 0) {
			return;
		}

		this->commodity_quantity += change;
	}

	virtual QString get_description() const = 0;

private:
	const metternich::commodity *commodity = nullptr;
	int amount = 0;
	int commodity_quantity = 0;
};

}
