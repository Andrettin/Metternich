#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class country;
class province;

class province_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner MEMBER owner)

public:
	explicit province_history(const province *province) : province(province)
	{
	}

	const country *get_owner() const
	{
		return this->owner;
	}

private:
	const metternich::province *province = nullptr;
	country *owner = nullptr;
};

}
