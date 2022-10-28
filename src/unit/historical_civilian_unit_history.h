#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class site;

class historical_civilian_unit_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::site* site MEMBER site)

public:
	const metternich::site *get_site() const
	{
		return this->site;
	}

	bool is_active() const
	{
		return this->get_site() != nullptr;
	}

private:
	metternich::site *site = nullptr;
};

}
