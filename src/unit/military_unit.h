#pragma once

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("unit/military_unit_type.h")

namespace metternich {

class character;
class country;
class culture;
class icon;
class military_unit_type;
class phenotype;
class population_type;
class promotion;
class province;
class religion;
class site;
enum class military_unit_category;
enum class military_unit_domain;

class military_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring NOTIFY name_changed)
	Q_PROPERTY(metternich::military_unit_type* type READ get_type_unconst NOTIFY type_changed)
	Q_PROPERTY(metternich::icon* icon READ get_icon_unconst NOTIFY icon_changed)
	Q_PROPERTY(metternich::country* country READ get_country_unconst CONSTANT)
	Q_PROPERTY(bool moving READ is_moving NOTIFY original_province_changed)
	Q_PROPERTY(metternich::site* site READ get_site_unconst NOTIFY site_changed)
	Q_PROPERTY(QVariantList promotions READ get_promotions_qvariant_list NOTIFY promotions_changed)

public:
	static constexpr int hit_point_recovery_per_turn = 10;
	static constexpr int morale_recovery_per_turn = 20;

	static const character *get_army_commander(const std::vector<military_unit *> &military_units);
	static const character *get_army_commander(const std::vector<const military_unit *> &military_units);
	static int get_army_score(const std::vector<military_unit *> &military_units);

	explicit military_unit(const military_unit_type *type);
	explicit military_unit(const military_unit_type *type, const metternich::country *country, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::site *home_settlement);
	explicit military_unit(const military_unit_type *type, const metternich::country *country, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::site *home_settlement);
	explicit military_unit(const military_unit_type *type, const character *character);

	void do_turn();
	void do_ai_turn();

	const std::string &get_name() const
	{
		return this->name;
	}

	QString get_name_qstring() const
	{
		return QString::fromStdString(this->get_name());
	}

	void set_name(const std::string &name)
	{
		if (name == this->get_name()) {
			return;
		}

		this->name = name;

		emit name_changed();
	}

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
	military_unit_domain get_domain() const;

	const icon *get_icon() const;

private:
	//for the Qt property (pointers there can't be const)
	icon *get_icon_unconst() const
	{
		return const_cast<icon *>(this->get_icon());
	}

public:
	const metternich::country *get_country() const
	{
		return this->country;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_country_unconst() const
	{
		return const_cast<metternich::country *>(this->get_country());
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

	const metternich::site *get_home_settlement() const
	{
		return this->home_settlement;
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

	int get_hit_points() const
	{
		return this->hit_points;
	}

	void set_hit_points(const int hit_points);

	void change_hit_points(const int change)
	{
		this->set_hit_points(this->get_hit_points() + change);
	}

	int get_max_hit_points() const
	{
		return this->max_hit_points;
	}

	void set_max_hit_points(const int max_hit_points)
	{
		if (max_hit_points == this->get_max_hit_points()) {
			return;
		}

		this->max_hit_points = max_hit_points;

		if (this->get_hit_points() > this->get_max_hit_points()) {
			this->set_hit_points(this->get_max_hit_points());
		}
	}

	void change_max_hit_points(const int change)
	{
		this->set_max_hit_points(this->get_max_hit_points() + change);
	}

	int get_morale() const
	{
		return this->morale;
	}

	void set_morale(const int morale)
	{
		if (morale == this->get_morale()) {
			return;
		}

		this->morale = morale;
	}

	void change_morale(const int change)
	{
		this->set_morale(this->get_morale() + change);
	}

	int get_morale_resistance() const;

	int get_melee() const
	{
		return this->melee;
	}

	void set_melee(const int melee)
	{
		if (melee == this->get_melee()) {
			return;
		}

		this->melee = melee;
	}

	void change_melee(const int change)
	{
		this->set_melee(this->get_melee() + change);
	}

	int get_defense() const
	{
		return this->defense;
	}

	void set_defense(const int defense)
	{
		if (defense == this->get_defense()) {
			return;
		}

		this->defense = defense;
	}

	void change_defense(const int change)
	{
		this->set_defense(this->get_defense() + change);
	}

	int get_damage_bonus() const
	{
		return this->damage_bonus;
	}

	void set_damage_bonus(const int bonus)
	{
		if (bonus == this->get_damage_bonus()) {
			return;
		}

		this->damage_bonus = bonus;
	}

	void change_damage_bonus(const int change)
	{
		this->set_damage_bonus(this->get_damage_bonus() + change);
	}

	int get_bonus_vs_infantry() const
	{
		return this->bonus_vs_infantry;
	}

	void set_bonus_vs_infantry(const int bonus)
	{
		if (bonus == this->get_bonus_vs_infantry()) {
			return;
		}

		this->bonus_vs_infantry = bonus;
	}

	void change_bonus_vs_infantry(const int change)
	{
		this->set_bonus_vs_infantry(this->get_bonus_vs_infantry() + change);
	}

	int get_bonus_vs_cavalry() const
	{
		return this->bonus_vs_cavalry;
	}

	void set_bonus_vs_cavalry(const int bonus)
	{
		if (bonus == this->get_bonus_vs_cavalry()) {
			return;
		}

		this->bonus_vs_cavalry = bonus;
	}

	void change_bonus_vs_cavalry(const int change)
	{
		this->set_bonus_vs_cavalry(this->get_bonus_vs_cavalry() + change);
	}

	int get_bonus_vs_artillery() const
	{
		return this->bonus_vs_artillery;
	}

	void set_bonus_vs_artillery(const int bonus)
	{
		if (bonus == this->get_bonus_vs_artillery()) {
			return;
		}

		this->bonus_vs_artillery = bonus;
	}

	void change_bonus_vs_artillery(const int change)
	{
		this->set_bonus_vs_artillery(this->get_bonus_vs_artillery() + change);
	}

	int get_bonus_vs_fortifications() const
	{
		return this->bonus_vs_fortifications;
	}

	void set_bonus_vs_fortifications(const int bonus)
	{
		if (bonus == this->get_bonus_vs_fortifications()) {
			return;
		}

		this->bonus_vs_fortifications = bonus;
	}

	void change_bonus_vs_fortifications(const int change)
	{
		this->set_bonus_vs_fortifications(this->get_bonus_vs_fortifications() + change);
	}

	int get_entrench_bonus_modifier() const
	{
		return this->entrench_bonus_modifier;
	}

	void set_entrench_bonus_modifier(const int bonus)
	{
		if (bonus == this->get_entrench_bonus_modifier()) {
			return;
		}

		this->entrench_bonus_modifier = bonus;
	}

	void change_entrench_bonus_modifier(const int change)
	{
		this->set_entrench_bonus_modifier(this->get_entrench_bonus_modifier() + change);
	}

	const std::vector<const promotion *> &get_promotions() const
	{
		return this->promotions;
	}

	QVariantList get_promotions_qvariant_list() const;
	bool can_have_promotion(const promotion *promotion) const;
	bool has_promotion(const promotion *promotion) const;
	void add_promotion(const promotion *promotion);
	void remove_promotion(const promotion *promotion);
	void check_promotions();
	void check_free_promotions();

	void receive_damage(const int damage);
	void heal(const int healing);

	void disband(const bool restore_population_unit);
	Q_INVOKABLE void disband();

	int get_score() const;

signals:
	void name_changed();
	void type_changed();
	void icon_changed();
	void province_changed();
	void original_province_changed();
	void site_changed();
	void promotions_changed();

private:
	std::string name;
	const military_unit_type *type = nullptr;
	const metternich::country *country = nullptr;
	const metternich::population_type *population_type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::site *home_settlement = nullptr;
	const metternich::character *character = nullptr;
	const metternich::province *province = nullptr; //the province the unit is in
	const metternich::province *original_province = nullptr; //the province before moving
	const metternich::site *site = nullptr; //the site the unit is visiting
	int hit_points = 0;
	int max_hit_points = 0;
	int morale = 0; //morale is never higher than the amount of hit points; when morale reaches zero, the unit flees in combat
	int melee = 0;
	int defense = 0;
	int damage_bonus = 0;
	int bonus_vs_infantry = 0;
	int bonus_vs_cavalry = 0;
	int bonus_vs_artillery = 0;
	int bonus_vs_fortifications = 0;
	int entrench_bonus_modifier = 0;
	std::vector<const promotion *> promotions;
};

}
