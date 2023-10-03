#pragma once

Q_MOC_INCLUDE("economy/commodity.h")

namespace metternich {

class commodity;

class transaction : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::commodity* commodity READ get_commodity CONSTANT)
	Q_PROPERTY(int amount READ get_amount NOTIFY amount_changed)

public:
	explicit transaction(const metternich::commodity *commodity, const int amount)
		: commodity(commodity), amount(amount)
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

		emit amount_changed();
	}

signals:
	void amount_changed();

private:
	const metternich::commodity *commodity = nullptr;
	int amount = 0;
};

}
