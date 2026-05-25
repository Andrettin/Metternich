#pragma once

Q_MOC_INCLUDE("economy/commodity.h")

namespace metternich {

class commodity;
class domain;
class icon;
class population_type;

class transaction : public QObject
{
	Q_OBJECT

		Q_PROPERTY(qint64 amount READ get_amount CONSTANT)
		Q_PROPERTY(qint64 object_quantity READ get_object_quantity CONSTANT)
		Q_PROPERTY(const metternich::domain* country READ get_country CONSTANT)
		Q_PROPERTY(const metternich::icon *icon READ get_icon CONSTANT)
		Q_PROPERTY(QString name READ get_name CONSTANT)
		Q_PROPERTY(QString description READ get_description CONSTANT)

public:
	using object_variant = std::variant<std::nullptr_t, const commodity *, const population_type *>;

	explicit transaction(const int64_t amount, const object_variant &object, const int64_t object_quantity, const metternich::domain *domain)
		: object(object), amount(amount), object_quantity(object_quantity), domain(domain)
	{
	}

	const object_variant &get_object() const
	{
		return this->object;
	}

	const std::string &get_object_name() const;

	int64_t get_amount() const
	{
		return this->amount;
	}

	void change_amount(const int64_t change)
	{
		if (change == 0) {
			return;
		}

		this->amount += change;
	}

	int64_t get_object_quantity() const
	{
		return this->object_quantity;
	}

	void change_object_quantity(const int64_t change)
	{
		if (change == 0) {
			return;
		}

		this->object_quantity += change;
	}

	const metternich::domain *get_country() const
	{
		return this->domain;
	}

	virtual QString get_name() const;
	virtual const icon *get_icon() const;
	virtual QString get_description() const = 0;

private:
	object_variant object{};
	int64_t amount = 0;
	int64_t object_quantity = 0;
	const metternich::domain *domain = nullptr;
};

}
