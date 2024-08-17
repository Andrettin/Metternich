#pragma once

Q_MOC_INCLUDE("economy/commodity.h")

namespace metternich {

class commodity;
class country;
class icon;
class population_type;

class transaction : public QObject
{
	Q_OBJECT

		Q_PROPERTY(int amount READ get_amount CONSTANT)
		Q_PROPERTY(int object_quantity READ get_object_quantity CONSTANT)
		Q_PROPERTY(const metternich::country *country READ get_country CONSTANT)
		Q_PROPERTY(const metternich::icon *icon READ get_icon CONSTANT)
		Q_PROPERTY(QString name READ get_name CONSTANT)
		Q_PROPERTY(QString description READ get_description CONSTANT)

public:
	using object_variant = std::variant<std::nullptr_t, const commodity *, const population_type *>;

	explicit transaction(const int amount, const object_variant &object, const int object_quantity, const metternich::country *country)
		: object(object), amount(amount), object_quantity(object_quantity), country(country)
	{
	}

	const object_variant &get_object() const
	{
		return this->object;
	}

	const std::string &get_object_name() const;

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

	int get_object_quantity() const
	{
		return this->object_quantity;
	}

	void change_object_quantity(const int change)
	{
		if (change == 0) {
			return;
		}

		this->object_quantity += change;
	}

	const metternich::country *get_country() const
	{
		return this->country;
	}

	virtual QString get_name() const;
	virtual const icon *get_icon() const;
	virtual QString get_description() const = 0;

private:
	object_variant object{};
	int amount = 0;
	int object_quantity = 0;
	const metternich::country *country = nullptr;
};

}
