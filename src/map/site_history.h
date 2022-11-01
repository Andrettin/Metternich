#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class improvement;
class site;

class site_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::improvement* improvement MEMBER improvement)

public:
	explicit site_history(const metternich::site *site) : site(site)
	{
	}

	const metternich::improvement *get_improvement() const
	{
		return this->improvement;
	}

private:
	const metternich::site *site = nullptr;
	metternich::improvement *improvement = nullptr;
};

}
