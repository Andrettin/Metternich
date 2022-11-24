#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"
#include "util/fractional_int.h"

namespace metternich {

class building_class;
class cultural_group;
class culture;
class employment_type;
class icon;

class building_type final : public named_data_entry, public data_type<building_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::building_class* building_class MEMBER building_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::employment_type* employment_type MEMBER employment_type NOTIFY changed)
	Q_PROPERTY(int employment_capacity MEMBER employment_capacity READ get_employment_capacity NOTIFY changed)
	Q_PROPERTY(int output_multiplier MEMBER output_multiplier READ get_output_multiplier NOTIFY changed)
	Q_PROPERTY(int housing MEMBER housing READ get_housing NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "building_type";
	static constexpr const char property_class_identifier[] = "metternich::building_type*";
	static constexpr const char database_folder[] = "building_types";

public:
	static constexpr int base_score = 50;

	explicit building_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

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

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const metternich::employment_type *get_employment_type() const
	{
		return this->employment_type;
	}

	int get_employment_capacity() const
	{
		return this->employment_capacity;
	}

	const commodity *get_output_commodity() const;

	int get_output_multiplier() const
	{
		return this->output_multiplier;
	}

	int get_housing() const
	{
		return this->housing;
	}

	int get_score() const
	{
		return building_type::base_score * std::max(1, this->get_employment_capacity() * this->get_output_multiplier());
	}

signals:
	void changed();

private:
	building_class *building_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::icon *icon = nullptr;
	metternich::employment_type *employment_type = nullptr;
	int employment_capacity = 0;
	int output_multiplier = 0;
	int housing = 0;
};

}
