#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "util/fractional_int.h"

namespace metternich {

class building_class;
class building_slot_type;
class country;
class cultural_group;
class culture;
class icon;
class population_unit;
class portrait;
class production_type;
class province;
class technology;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class building_type final : public named_data_entry, public data_type<building_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_class* building_class MEMBER building_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(bool provincial MEMBER provincial READ is_provincial NOTIFY changed)
	Q_PROPERTY(QVariantList production_types READ get_production_types_qvariant_list NOTIFY changed)
	Q_PROPERTY(int base_capacity MEMBER base_capacity READ get_base_capacity NOTIFY changed)
	Q_PROPERTY(int capacity_increment MEMBER capacity_increment READ get_capacity_increment NOTIFY changed)
	Q_PROPERTY(bool warehouse MEMBER warehouse READ is_warehouse NOTIFY changed)
	Q_PROPERTY(bool expandable MEMBER expandable READ is_expandable NOTIFY changed)
	Q_PROPERTY(int max_level MEMBER max_level READ get_max_level NOTIFY changed)
	Q_PROPERTY(metternich::building_type* required_building MEMBER required_building NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(QString country_modifier_string READ get_country_modifier_string CONSTANT)

public:
	static constexpr const char class_identifier[] = "building_type";
	static constexpr const char property_class_identifier[] = "metternich::building_type*";
	static constexpr const char database_folder[] = "building_types";

public:
	static constexpr int base_score = 10;

	explicit building_type(const std::string &identifier);
	~building_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const building_class *get_building_class() const
	{
		return this->building_class;
	}

	const building_slot_type *get_slot_type() const;

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::cultural_group *get_cultural_group() const
	{
		return this->cultural_group;
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_provincial() const
	{
		return this->provincial;
	}

	const std::vector<const production_type *> &get_production_types() const
	{
		return this->production_types;
	}

	QVariantList get_production_types_qvariant_list() const;

	int get_base_capacity() const
	{
		return this->base_capacity;
	}

	int get_capacity_increment() const
	{
		return this->capacity_increment;
	}

	bool is_warehouse() const
	{
		return this->warehouse;
	}

	bool is_expandable() const
	{
		return this->expandable;
	}

	int get_max_level() const
	{
		return this->max_level;
	}

	const building_type *get_required_building() const
	{
		return this->required_building;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	int get_score() const;

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const condition<province> *get_province_conditions() const
	{
		return this->province_conditions.get();
	}

	const modifier<const country> *get_country_modifier() const
	{
		return this->country_modifier.get();
	}

	const modifier<const country> *get_stackable_country_modifier() const
	{
		return this->stackable_country_modifier.get();
	}

	QString get_country_modifier_string() const;

signals:
	void changed();

private:
	building_class *building_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::portrait *portrait = nullptr;
	metternich::icon *icon = nullptr;
	bool provincial = false;
	std::vector<const production_type *> production_types;
	int base_capacity = 0;
	int capacity_increment = 0;
	bool warehouse = false;
	bool expandable = false;
	int max_level = 0;
	building_type *required_building = nullptr;
	technology *required_technology = nullptr;
	std::unique_ptr<const condition<country>> conditions;
	std::unique_ptr<const condition<province>> province_conditions;
	std::unique_ptr<modifier<const country>> country_modifier;
	std::unique_ptr<modifier<const country>> stackable_country_modifier;
};

}
