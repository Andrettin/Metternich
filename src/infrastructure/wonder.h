#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("infrastructure/building_type.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class building_type;
class country;
class portrait;
class province;
class technology;

template <typename scope_type>
class condition;

template <typename scope_type>
class factor;

template <typename scope_type>
class modifier;

class wonder final : public named_data_entry, public data_type<wonder>
{
	Q_OBJECT

	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::building_type* building MEMBER building NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)
	Q_PROPERTY(int wealth_cost MEMBER wealth_cost READ get_wealth_cost NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "wonder";
	static constexpr const char property_class_identifier[] = "metternich::wonder*";
	static constexpr const char database_folder[] = "wonders";

	explicit wonder(const std::string &identifier);
	~wonder();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const building_type *get_building() const
	{
		return this->building;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

	int get_wealth_cost() const
	{
		return this->wealth_cost;
	}

	int get_wealth_cost_for_country(const country *country) const;

	const commodity_map<int> &get_commodity_costs() const
	{
		return this->commodity_costs;
	}

	commodity_map<int> get_commodity_costs_for_country(const country *country) const;

	const factor<country> *get_cost_factor() const
	{
		return this->cost_factor.get();
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const condition<province> *get_province_conditions() const
	{
		return this->province_conditions.get();
	}

	const modifier<const province> *get_province_modifier() const
	{
		return this->province_modifier.get();
	}

	const modifier<const country> *get_country_modifier() const
	{
		return this->country_modifier.get();
	}

signals:
	void changed();

private:
	metternich::portrait *portrait = nullptr;
	building_type *building = nullptr;
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	int wealth_cost = 0;
	commodity_map<int> commodity_costs;
	std::unique_ptr<const factor<country>> cost_factor;
	std::unique_ptr<const condition<country>> conditions;
	std::unique_ptr<const condition<province>> province_conditions;
	std::unique_ptr<modifier<const province>> province_modifier;
	std::unique_ptr<modifier<const country>> country_modifier;
};

}
