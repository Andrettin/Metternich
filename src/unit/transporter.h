#pragma once

#include "util/fractional_int.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("unit/transporter_type.h")

namespace metternich {

class country;
class culture;
class icon;
class phenotype;
class population_type;
class religion;
class site;
class transporter_type;
enum class transporter_category;
enum class transporter_stat;

class transporter final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString name READ get_name_qstring NOTIFY name_changed)
	Q_PROPERTY(const metternich::transporter_type* type READ get_type NOTIFY type_changed)
	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY icon_changed)
	Q_PROPERTY(const metternich::country* country READ get_country CONSTANT)

public:
	explicit transporter(const transporter_type *type, const metternich::country *country, const metternich::population_type *population_type, const metternich::culture *culture, const metternich::religion *religion, const metternich::phenotype *phenotype, const metternich::site *home_settlement);

	void do_turn();

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

	const transporter_type *get_type() const
	{
		return this->type;
	}

	void set_type(const transporter_type *type);

	transporter_category get_category() const;
	bool is_ship() const;

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

	const centesimal_int &get_stat(const transporter_stat stat) const
	{
		const auto find_iterator = this->stats.find(stat);
		if (find_iterator != this->stats.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_stat(const transporter_stat stat, const centesimal_int &value)
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

	void change_stat(const transporter_stat stat, const centesimal_int &change)
	{
		this->set_stat(stat, this->get_stat(stat) + change);
	}

	int get_discipline() const;

	int get_cargo() const;

	void receive_damage(const int damage, const int morale_damage_modifier);
	void heal(const int healing);

	void disband(const bool dead);
	Q_INVOKABLE void disband();

	int get_score() const;

signals:
	void name_changed();
	void type_changed();
	void icon_changed();

private:
	std::string name;
	const transporter_type *type = nullptr;
	const metternich::country *country = nullptr;
	const metternich::population_type *population_type = nullptr;
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::phenotype *phenotype = nullptr;
	const metternich::site *home_settlement = nullptr;
	int hit_points = 0;
	int max_hit_points = 0;
	int morale = 0; //morale is never higher than the amount of hit points; when morale reaches zero, the unit flees in combat
	std::map<transporter_stat, centesimal_int> stats;
};

}
