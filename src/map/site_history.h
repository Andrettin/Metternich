#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class site;

class site_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(int development_level MEMBER development_level READ get_development_level)

public:
	explicit site_history(const metternich::site *site) : site(site)
	{
	}

	int get_development_level() const
	{
		return this->development_level;
	}

private:
	const metternich::site *site = nullptr;
	int development_level = 0;
};

}
