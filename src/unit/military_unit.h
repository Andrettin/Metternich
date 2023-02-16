#pragma once

namespace metternich {

class character;
class country;
class culture;
class icon;
class military_unit_type;
class phenotype;
class population_type;
class province;
class religion;
class site;
enum class military_unit_category;

class military_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(metternich::military_unit_type* type READ get_type_unconst NOTIFY type_changed)
	Q_PROPERTY(metternich::icon* icon READ get_icon_unconst NOTIFY icon_changed)
	Q_PROPERTY(metternich::country* owner READ get_owner_unconst CONSTANT)
	Q_PROPERTY(metternich::province* home_province READ get_home_province_unconst CONSTANT)
	Q_PROPERTY(bool moving READ is_moving NOTIFY original_province_changed)
	Q_PROPERTY(metternich::site* site READ get_site_unconst NOTIFY site_changed)

public:
	static const character *get_army_commander(const std::vector<military_unit *> &military_units);
	static const character *get_army_commander(const std::vector<const military_unit *> &military_units);

	explicit military_unit(const military_unit_type *type, const country *owner, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype);
	explicit military_unit(const military_unit_type *type, const country *owner, const metternich::province *home_province, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype);
	explicit military_unit(const military_unit_type *type, const character *character);

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
	void set_type(const military_unit_type *type);

	military_unit_category get_category() const;

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

	const metternich::character *get_character() const
	{
		return this->character;
	}

	const metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(const metternich::province *province);

	const metternich::province *get_original_province() const
	{
		return this->original_province;
	}

	void set_original_province(const metternich::province *province)
	{
		if (province == this->get_original_province()) {
			return;
		}

		this->original_province = province;
		emit original_province_changed();
	}

	bool can_move_to(const metternich::province *province) const;
	void move_to(const metternich::province *province);
	void cancel_move();

	bool is_moving() const
	{
		return this->get_original_province() != nullptr;
	}

	const metternich::site *get_site() const
	{
		return this->site;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::site *get_site_unconst() const
	{
		return const_cast<metternich::site *>(this->get_site());
	}

public:
	void set_site(const metternich::site *site);
	void visit_site(const metternich::site *site);

	void disband(const bool restore_population_unit);
	Q_INVOKABLE void disband();

signals:
	void type_changed();
	void icon_changed();
	void province_changed();
	void original_province_changed();
	void site_changed();

private:
	const military_unit_type *type = nullptr;
	const country *owner = nullptr;
	const province *home_province = nullptr;
	const metternich::population_type *population_type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::character *character = nullptr;
	const metternich::province *province = nullptr; //the province the unit is in
	const metternich::province *original_province = nullptr; //the province before moving
	const metternich::site *site = nullptr; //the site the unit is visiting
};

}
