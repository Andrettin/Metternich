#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class province;

class historical_military_unit_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::province* province MEMBER province)

public:
	const metternich::province *get_province() const
	{
		return this->province;
	}

	bool is_active() const
	{
		return this->get_province() != nullptr;
	}

private:
	metternich::province *province = nullptr;
};

}
