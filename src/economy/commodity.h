#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("technology/technology.h")

namespace metternich {

class icon;
class technology;
enum class food_type;

class commodity final : public named_data_entry, public data_type<commodity>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::food_type food_type MEMBER food_type READ get_food_type NOTIFY changed)
	Q_PROPERTY(bool abstract MEMBER abstract READ is_abstract NOTIFY changed)
	Q_PROPERTY(bool storable MEMBER storable READ is_storable NOTIFY changed)
	Q_PROPERTY(bool local MEMBER local READ is_local NOTIFY changed)
	Q_PROPERTY(bool negative_allowed MEMBER negative_allowed READ is_negative_allowed NOTIFY changed)
	Q_PROPERTY(bool labor MEMBER labor READ is_labor NOTIFY changed)
	Q_PROPERTY(bool health MEMBER health READ is_health NOTIFY changed)
	Q_PROPERTY(int wealth_value MEMBER wealth_value READ get_wealth_value NOTIFY changed)
	Q_PROPERTY(int base_price MEMBER base_price READ get_base_price NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "commodity";
	static constexpr const char property_class_identifier[] = "metternich::commodity*";
	static constexpr const char database_folder[] = "commodities";

	static constexpr int abstract_commodity_value = 100; //used for e.g. score calculation

	explicit commodity(const std::string &identifier);

	virtual void initialize() override;
	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_food() const;

	metternich::food_type get_food_type() const
	{
		return this->food_type;
	}

	bool is_abstract() const
	{
		return this->abstract;
	}

	bool is_storable() const
	{
		return this->storable;
	}

	bool is_local() const
	{
		return this->local;
	}

	bool is_negative_allowed() const
	{
		return this->negative_allowed;
	}

	bool is_labor() const
	{
		return this->labor;
	}

	bool is_health() const
	{
		return this->health;
	}

	int get_wealth_value() const
	{
		return this->wealth_value;
	}

	bool is_convertible_to_wealth() const
	{
		return this->get_wealth_value() != 0;
	}

	int get_base_price() const
	{
		return this->base_price;
	}

	bool is_tradeable() const
	{
		return !this->is_abstract() && this->is_storable() && !this->is_convertible_to_wealth();
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

signals:
	void changed();

private:
	metternich::icon *icon = nullptr;
	metternich::food_type food_type;
	bool abstract = false;
	bool storable = true;
	bool local = false;
	bool negative_allowed = false;
	bool labor = false;
	bool health = false;
	int wealth_value = 0;
	int base_price = 0;
	technology *required_technology = nullptr;
};

}
