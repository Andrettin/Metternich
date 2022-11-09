#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class country;
class province;

class province_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner MEMBER owner)
	Q_PROPERTY(int population MEMBER population READ get_population)

public:
	explicit province_history(const province *province) : province(province)
	{
	}

	const country *get_owner() const
	{
		return this->owner;
	}

	int get_population() const
	{
		return this->population;
	}

	void set_population(const int population)
	{
		this->population = population;
	}

private:
	const metternich::province *province = nullptr;
	country *owner = nullptr;
	int population = 0;
};

}
