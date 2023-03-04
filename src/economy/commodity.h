#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;
enum class food_type;

class commodity final : public named_data_entry, public data_type<commodity>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::food_type food_type MEMBER food_type READ get_food_type NOTIFY changed)
	Q_PROPERTY(int wealth_value MEMBER wealth_value READ get_wealth_value NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "commodity";
	static constexpr const char property_class_identifier[] = "metternich::commodity*";
	static constexpr const char database_folder[] = "commodities";

	explicit commodity(const std::string &identifier);

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

	int get_wealth_value() const
	{
		return this->wealth_value;
	}

	bool is_convertible_to_wealth() const
	{
		return this->get_wealth_value() != 0;
	}

signals:
	void changed();

private:
	metternich::icon *icon = nullptr;
	metternich::food_type food_type;
	int wealth_value = 0;
};

}
