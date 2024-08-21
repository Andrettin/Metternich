#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "unit/military_unit_type_container.h"
#include "util/fractional_int.h"

Q_MOC_INCLUDE("country/cultural_group.h")
Q_MOC_INCLUDE("country/culture.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("unit/military_unit_class.h")

namespace metternich {

class military_unit_class;
class country;
class cultural_group;
class culture;
class icon;
class promotion;
class technology;
enum class military_unit_category;
enum class military_unit_domain;
enum class military_unit_stat;

class military_unit_type final : public named_data_entry, public data_type<military_unit_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::military_unit_class* unit_class MEMBER unit_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(int hit_points MEMBER hit_points READ get_hit_points NOTIFY changed)
	Q_PROPERTY(bool entrench MEMBER entrench READ can_entrench NOTIFY changed)
	Q_PROPERTY(int entrenchment_bonus MEMBER entrenchment_bonus READ get_entrenchment_bonus NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(int wealth_cost MEMBER wealth_cost READ get_wealth_cost NOTIFY changed)
	Q_PROPERTY(int upkeep MEMBER upkeep READ get_upkeep NOTIFY changed)

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

	const std::map<military_unit_stat, centesimal_int> &get_stats() const
	{
		return this->stats;
	}

	const centesimal_int &get_stat(const military_unit_stat stat) const
	{
		const auto find_iterator = this->get_stats().find(stat);
		if (find_iterator != this->get_stats().end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	centesimal_int get_stat_for_country(const military_unit_stat stat, const country *country) const;

	int get_hit_points() const
	{
		return this->hit_points;
	}

	bool can_entrench() const
	{
		return this->entrench;
	}

	int get_entrenchment_bonus() const
	{
		return this->entrenchment_bonus;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	int get_wealth_cost() const
	{
		return this->wealth_cost;
	}

	int get_upkeep() const
	{
		return this->upkeep;
	}

	const commodity_map<int> &get_commodity_costs() const
	{
		return this->commodity_costs;
	}

	const std::vector<const promotion *> &get_free_promotions() const
	{
		return this->free_promotions;
	}

	const military_unit_type_set &get_upgrades() const
	{
		return this->upgrades;
	}

	int get_score() const;

signals:
	void changed();

private:
	military_unit_class *unit_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::icon *icon = nullptr;
	std::map<military_unit_stat, centesimal_int> stats;
	int hit_points = 25;
	bool entrench = false;
	int entrenchment_bonus = 1; //the entrenchment bonus to defense
	technology *required_technology = nullptr;
	int wealth_cost = 0;
	commodity_map<int> commodity_costs;
	int upkeep = 0; //wealth paid per turn as upkeep for the military unit
	std::vector<const promotion *> free_promotions;
	military_unit_type_set upgrades;
};

}
