#pragma once

#include "util/fractional_int.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("map/site.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("unit/military_unit_type.h")

namespace metternich {

class army;
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
enum class military_unit_stat;

class military_unit final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring NOTIFY name_changed)
	Q_PROPERTY(const metternich::military_unit_type* type READ get_type NOTIFY type_changed)
	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY icon_changed)
	Q_PROPERTY(const metternich::country* country READ get_country CONSTANT)
	Q_PROPERTY(bool moving READ is_moving NOTIFY army_changed)
	Q_PROPERTY(QVariantList promotions READ get_promotions_qvariant_list NOTIFY promotions_changed)
	Q_PROPERTY(int hit_points READ get_hit_points NOTIFY hit_points_changed)
	Q_PROPERTY(int max_hit_points READ get_max_hit_points NOTIFY max_hit_points_changed)
	Q_PROPERTY(int morale READ get_morale NOTIFY morale_changed)

public:
	static constexpr int hit_point_recovery_per_turn = 10;
	static constexpr int morale_recovery_per_turn = 20;

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

	void generate_name();

	const military_unit_type *get_type() const
	{
		return this->type;
	}

	void set_type(const military_unit_type *type);

	military_unit_category get_category() const;
	military_unit_domain get_domain() const;

	const icon *get_icon() const;

	const metternich::country *get_country() const
	{
		return this->country;
	}

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

	const metternich::army *get_army() const
	{
		return this->army;
	}

	void set_army(metternich::army *army);

	bool can_move_to(const metternich::province *province) const;

	bool is_moving() const
	{
		return this->get_army() != nullptr;
	}

	bool is_hostile_to(const metternich::country *country) const;

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

		emit max_hit_points_changed();
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

		emit morale_changed();
	}

	void change_morale(const int change)
	{
		this->set_morale(this->get_morale() + change);
	}

	int get_discipline() const;

	const centesimal_int &get_stat(const military_unit_stat stat) const
	{
		const auto find_iterator = this->stats.find(stat);
		if (find_iterator != this->stats.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_stat(const military_unit_stat stat, const centesimal_int &value)
	{
		if (value == this->get_stat(stat)) {
			return;
		}

		if (value == 0) {
			this->stats.erase(stat);
		} else {
			this->stats[stat] = value;
		}
	}

	void change_stat(const military_unit_stat stat, const centesimal_int &change)
	{
		this->set_stat(stat, this->get_stat(stat) + change);
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

	void attack(military_unit *target, const bool ranged, const bool target_entrenched);
	void receive_damage(const int damage, const int morale_damage_modifier);
	void heal(const int healing);

	void disband(const bool dead);
	Q_INVOKABLE void disband();

	int get_score() const;

signals:
	void name_changed();
	void type_changed();
	void icon_changed();
	void province_changed();
	void army_changed();
	void promotions_changed();
	void hit_points_changed();
	void max_hit_points_changed();
	void morale_changed();

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
	metternich::army *army = nullptr; //the army to which the unit belongs
	int hit_points = 0;
	int max_hit_points = 0;
	int morale = 0; //morale is never higher than the amount of hit points; when morale reaches zero, the unit flees in combat
	std::map<military_unit_stat, centesimal_int> stats;
	std::vector<const promotion *> promotions;
};

}
