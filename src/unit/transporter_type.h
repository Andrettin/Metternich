#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class cultural_group;
class culture;
class icon;
class technology;
class transporter_class;
enum class transporter_category;

class transporter_type final : public named_data_entry, public data_type<transporter_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::transporter_class* transporter_class MEMBER transporter_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int defense MEMBER defense READ get_defense NOTIFY changed)
	Q_PROPERTY(int resistance MEMBER resistance READ get_resistance NOTIFY changed)
	Q_PROPERTY(int hit_points MEMBER hit_points READ get_hit_points NOTIFY changed)
	Q_PROPERTY(int speed MEMBER speed READ get_speed NOTIFY changed)
	Q_PROPERTY(int cargo MEMBER cargo READ get_cargo NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "transporter_type";
	static constexpr const char property_class_identifier[] = "metternich::transporter_type*";
	static constexpr const char database_folder[] = "transporter_types";

public:
	explicit transporter_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const metternich::transporter_class *get_transporter_class() const
	{
		return this->transporter_class;
	}

	transporter_category get_category() const;
	bool is_ship() const;

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::cultural_group *get_cultural_group() const
	{
		return this->cultural_group;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	int get_defense() const
	{
		return this->defense;
	}

	int get_resistance() const
	{
		return this->resistance;
	}

	int get_hit_points() const
	{
		return this->hit_points;
	}

	int get_speed() const
	{
		return this->speed;
	}

	int get_cargo() const
	{
		return this->cargo;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	int get_score() const
	{
		return this->get_defense() + this->get_resistance() / 4 + this->get_hit_points() + this->get_speed() + this->get_cargo();
	}

signals:
	void changed();

private:
	metternich::transporter_class *transporter_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::icon *icon = nullptr;
	int defense = 0;
	int resistance = 0; //resistance to damage, in percent
	int hit_points = 25;
	int speed = 0;
	int cargo = 0;
	technology *required_technology = nullptr;
};

}