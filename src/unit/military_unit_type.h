#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class military_unit_class;
class cultural_group;
class culture;
class icon;
class promotion;
class technology;
enum class military_unit_category;
enum class military_unit_domain;

class military_unit_type final : public named_data_entry, public data_type<military_unit_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::military_unit_class* unit_class MEMBER unit_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int firepower MEMBER firepower READ get_firepower NOTIFY changed)
	Q_PROPERTY(int melee MEMBER melee READ get_melee NOTIFY changed)
	Q_PROPERTY(int range MEMBER range READ get_range NOTIFY changed)
	Q_PROPERTY(int defense MEMBER defense READ get_defense NOTIFY changed)
	Q_PROPERTY(int resistance MEMBER resistance READ get_resistance NOTIFY changed)
	Q_PROPERTY(int hit_points MEMBER hit_points READ get_hit_points NOTIFY changed)
	Q_PROPERTY(int movement MEMBER movement READ get_movement NOTIFY changed)
	Q_PROPERTY(bool entrench MEMBER entrench READ can_entrench NOTIFY changed)
	Q_PROPERTY(int entrench_bonus MEMBER entrench_bonus READ get_entrench_bonus NOTIFY changed)
	Q_PROPERTY(int bonus_vs_infantry MEMBER bonus_vs_infantry READ get_bonus_vs_infantry NOTIFY changed)
	Q_PROPERTY(int bonus_vs_cavalry MEMBER bonus_vs_cavalry READ get_bonus_vs_cavalry NOTIFY changed)
	Q_PROPERTY(int bonus_vs_artillery MEMBER bonus_vs_artillery READ get_bonus_vs_artillery NOTIFY changed)
	Q_PROPERTY(int bonus_vs_fortifications MEMBER bonus_vs_fortifications READ get_bonus_vs_fortifications NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "military_unit_type";
	static constexpr const char property_class_identifier[] = "metternich::military_unit_type*";
	static constexpr const char database_folder[] = "military_unit_types";

public:
	explicit military_unit_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const military_unit_class *get_unit_class() const
	{
		return this->unit_class;
	}

	military_unit_category get_category() const;
	military_unit_domain get_domain() const;

	bool is_infantry() const;
	bool is_cavalry() const;
	bool is_artillery() const;

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

	int get_firepower() const
	{
		return this->firepower;
	}

	int get_melee() const
	{
		return this->melee;
	}

	int get_range() const
	{
		return this->range;
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

	int get_movement() const
	{
		return this->movement;
	}

	bool can_entrench() const
	{
		return this->entrench;
	}

	int get_entrench_bonus() const
	{
		return this->entrench_bonus;
	}

	int get_bonus_vs_infantry() const
	{
		return this->bonus_vs_infantry;
	}

	int get_bonus_vs_cavalry() const
	{
		return this->bonus_vs_cavalry;
	}

	int get_bonus_vs_artillery() const
	{
		return this->bonus_vs_artillery;
	}

	int get_bonus_vs_fortifications() const
	{
		return this->bonus_vs_fortifications;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const std::vector<const promotion *> &get_free_promotions() const
	{
		return this->free_promotions;
	}

	int get_score() const
	{
		return this->get_firepower() + this->get_melee() + this->get_range() + this->get_defense() + this->get_resistance() / 4 + this->get_hit_points() + this->get_movement();
	}

signals:
	void changed();

private:
	military_unit_class *unit_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::icon *icon = nullptr;
	int firepower = 0;
	int melee = 0;
	int range = 0;
	int defense = 0;
	int resistance = 0; //resistance to damage, in percent
	int hit_points = 25;
	int movement = 0;
	bool entrench = false;
	int entrench_bonus = 1; //the entrenchment bonus to defense
	int bonus_vs_infantry = 0;
	int bonus_vs_cavalry = 0;
	int bonus_vs_artillery = 0;
	int bonus_vs_fortifications = 0;
	technology *required_technology = nullptr;
	std::vector<const promotion *> free_promotions;
};

}
