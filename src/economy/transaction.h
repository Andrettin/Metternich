#pragma once

Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("economy/commodity.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class commodity;
class domain;
class icon;
class population_type;

class transaction : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::domain* domain READ get_domain CONSTANT)
	Q_PROPERTY(const metternich::icon* icon READ get_icon CONSTANT)
	Q_PROPERTY(QString name READ get_name CONSTANT)
	Q_PROPERTY(QString description READ get_description CONSTANT)

public:
	using object_variant = std::variant<std::nullptr_t, const commodity *, const population_type *>;

	explicit transaction(const decimillesimal_int &amount, const object_variant &object, const decimillesimal_int &object_quantity, const metternich::domain *domain)
		: object(object), amount(amount), object_quantity(object_quantity), domain(domain)
	{
	}

	const object_variant &get_object() const
	{
		return this->object;
	}

	const std::string &get_object_name() const;

	const decimillesimal_int &get_amount() const
	{
		return this->amount;
	}

	void change_amount(const decimillesimal_int &change)
	{
		if (change == 0) {
			return;
		}

		this->amount += change;
	}

	const decimillesimal_int &get_object_quantity() const
	{
		return this->object_quantity;
	}

	void change_object_quantity(const decimillesimal_int &change)
	{
		if (change == 0) {
			return;
		}

		this->object_quantity += change;
	}

	const metternich::domain *get_domain() const
	{
		return this->domain;
	}

	virtual QString get_name() const;
	virtual const icon *get_icon() const;
	virtual QString get_description() const = 0;

private:
	object_variant object{};
	decimillesimal_int amount;
	decimillesimal_int object_quantity;
	const metternich::domain *domain = nullptr;
};

}
