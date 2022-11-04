#pragma once

#include "country/country_container.h"
#include "database/data_entry_history.h"

namespace metternich {

class country;
class technology;
enum class diplomacy_state;

class country_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(std::vector<metternich::technology *> technologies READ get_technologies)

public:
	explicit country_history(const metternich::country *country) : country(country)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const std::vector<technology *> &get_technologies() const
	{
		return this->technologies;
	}

	Q_INVOKABLE void add_technology(technology *technology)
	{
		this->technologies.push_back(technology);
	}

	Q_INVOKABLE void remove_region(technology *technology)
	{
		std::erase(this->technologies, technology);
	}


	const country_map<diplomacy_state> &get_diplomacy_states() const
	{
		return this->diplomacy_states;
	}

private:
	const metternich::country *country = nullptr;
	std::vector<technology *> technologies;
	country_map<diplomacy_state> diplomacy_states;
};

}
