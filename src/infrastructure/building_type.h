#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "util/fractional_int.h"

namespace metternich {

class building_class;
class country;
class cultural_group;
class culture;
class icon;
class population_unit;
class production_type;
class technology;

template <typename scope_type>
class modifier;

class building_type final : public named_data_entry, public data_type<building_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_class* building_class MEMBER building_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::icon* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(QVariantList production_types READ get_production_types_qvariant_list NOTIFY changed)
	Q_PROPERTY(int base_capacity MEMBER base_capacity READ get_base_capacity NOTIFY changed)
	Q_PROPERTY(bool warehouse MEMBER warehouse READ is_warehouse NOTIFY changed)
	Q_PROPERTY(bool expandable MEMBER expandable READ is_expandable NOTIFY changed)
	Q_PROPERTY(metternich::building_type* required_building MEMBER required_building NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

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

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::cultural_group *get_cultural_group() const
	{
		return this->cultural_group;
	}

	const metternich::icon *get_portrait() const
	{
		return this->portrait;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
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

	bool is_warehouse() const
	{
		return this->warehouse;
	}

	bool is_expandable() const
	{
		return this->expandable;
	}

	const building_type *get_required_building() const
	{
		return this->required_building;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	int get_score() const
	{
		return (building_type::base_score * std::max(1, this->get_base_capacity()));
	}

	const modifier<const country> *get_country_modifier() const
	{
		return this->country_modifier.get();
	}

signals:
	void changed();

private:
	building_class *building_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::icon *portrait = nullptr;
	metternich::icon *icon = nullptr;
	std::vector<const production_type *> production_types;
	int base_capacity = 0;
	bool warehouse = false;
	bool expandable = false;
	building_type *required_building = nullptr;
	technology *required_technology = nullptr;
	std::unique_ptr<modifier<const country>> country_modifier;
};

}
