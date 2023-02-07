#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class technology;

class culture_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(std::vector<const technology *> technologies READ get_technologies)

public:
	const std::vector<const technology *> &get_technologies() const
	{
		return this->technologies;
	}

	Q_INVOKABLE void add_technology(const technology *technology)
	{
		this->technologies.push_back(technology);
	}

	Q_INVOKABLE void remove_technology(const technology *technology)
	{
		std::erase(this->technologies, technology);
	}

private:
	std::vector<const technology *> technologies;
};

}
