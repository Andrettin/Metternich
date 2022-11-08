#pragma once

namespace metternich {

class icon;
class population_type;
class province;

class population_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::population_type* type READ get_type_unconst NOTIFY type_changed)
	Q_PROPERTY(metternich::icon* icon READ get_icon_unconst NOTIFY icon_changed)
	Q_PROPERTY(metternich::province* province READ get_province_unconst NOTIFY province_changed)

public:
	explicit population_unit(const population_type *type, const metternich::province *province);

	const population_type *get_type() const
	{
		return this->type;
	}

private:
	//for the Qt property (pointers there can't be const)
	population_type *get_type_unconst() const
	{
		return const_cast<population_type *>(this->get_type());
	}

public:
	void set_type(const population_type *type)
	{
		if (type == this->get_type()) {
			return;
		}

		this->type = type;
		emit type_changed();
	}

	const icon *get_icon() const;

private:
	//for the Qt property (pointers there can't be const)
	icon *get_icon_unconst() const
	{
		return const_cast<icon *>(this->get_icon());
	}

public:
	const metternich::province *get_province() const
	{
		return this->province;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::province *get_province_unconst() const
	{
		return const_cast<metternich::province *>(this->get_province());
	}

public:
	void set_province(const metternich::province *province);

signals:
	void type_changed();
	void icon_changed();
	void province_changed();

private:
	const population_type *type = nullptr;
	const metternich::province *province = nullptr;
};

}
