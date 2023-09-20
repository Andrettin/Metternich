#pragma once

#include "database/data_entry_history.h"

Q_MOC_INCLUDE("map/province.h")

namespace metternich {

class promotion;
class province;

class historical_military_unit_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::province* province MEMBER province)
	Q_PROPERTY(std::vector<metternich::promotion *> promotions READ get_promotions)

public:
	const metternich::province *get_province() const
	{
		return this->province;
	}

	bool is_active() const
	{
		return this->get_province() != nullptr;
	}

	const std::vector<promotion *> &get_promotions() const
	{
		return this->promotions;
	}

	Q_INVOKABLE void add_promotion(metternich::promotion *promotion)
	{
		this->promotions.push_back(promotion);
	}

	Q_INVOKABLE void remove_promotion(metternich::promotion *promotion)
	{
		std::erase(this->promotions, promotion);
	}

private:
	metternich::province *province = nullptr;
	std::vector<promotion *> promotions;
};

}
