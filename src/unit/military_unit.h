#pragma once

namespace metternich {

class country;
class culture;
class icon;
class military_unit_type;
class phenotype;
class population_type;
class province;
class religion;

class military_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::military_unit_type* type READ get_type_unconst NOTIFY type_changed)
	Q_PROPERTY(metternich::icon* icon READ get_icon_unconst NOTIFY icon_changed)
	Q_PROPERTY(metternich::country* owner READ get_owner_unconst CONSTANT)
	Q_PROPERTY(metternich::province* home_province READ get_home_province_unconst CONSTANT)
	Q_PROPERTY(bool moving READ is_moving NOTIFY original_province_changed)

public:
	explicit military_unit(const military_unit_type *type, const country *owner, const metternich::province *home_province, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype);

	void do_turn();
	void do_ai_turn();

	const military_unit_type *get_type() const
	{
		return this->type;
	}

private:
	//for the Qt property (pointers there can't be const)
	military_unit_type *get_type_unconst() const
	{
		return const_cast<military_unit_type *>(this->get_type());
	}

public:
	void set_type(const military_unit_type *type)
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
	const country *get_owner() const
	{
		return this->owner;
	}

private:
	//for the Qt property (pointers there can't be const)
	country *get_owner_unconst() const
	{
		return const_cast<country *>(this->get_owner());
	}

public:
	const metternich::province *get_home_province() const
	{
		return this->home_province;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::province *get_home_province_unconst() const
	{
		return const_cast<metternich::province *>(this->get_home_province());
	}

public:
	const metternich::population_type *get_population_type() const
	{
		return this->population_type;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	const metternich::phenotype *get_phenotype() const
	{
		return this->phenotype;
	}

	const metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(const metternich::province *province);

	void set_original_province(const metternich::province *province)
	{
		if (province == this->original_province) {
			return;
		}

		this->original_province = province;
		emit original_province_changed();
	}

	Q_INVOKABLE bool can_move_to(metternich::province *province) const;
	Q_INVOKABLE void move_to(metternich::province *province);
	Q_INVOKABLE void cancel_move();

	bool is_moving() const
	{
		return this->original_province != nullptr;
	}

	void disband(const bool restore_population_unit);
	Q_INVOKABLE void disband();

signals:
	void type_changed();
	void icon_changed();
	void province_changed();
	void original_province_changed();

private:
	const military_unit_type *type = nullptr;
	const country *owner = nullptr;
	const province *home_province = nullptr;
	const metternich::population_type *population_type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::province *province = nullptr; //the province the unit is in
	const metternich::province *original_province = nullptr; //the province before moving
};

}
