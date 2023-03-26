#pragma once

#include "country/country_container.h"
#include "database/data_entry_history.h"
#include "util/fractional_int.h"

namespace metternich {

class character;
class consulate;
class country;
class religion;
class technology;
enum class diplomacy_state;

class country_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::religion* religion MEMBER religion)
	Q_PROPERTY(archimedes::centesimal_int literacy_rate MEMBER literacy_rate READ get_literacy_rate)
	Q_PROPERTY(std::vector<metternich::technology *> technologies READ get_technologies)

public:
	explicit country_history(const metternich::country *country);

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const religion *get_religion() const
	{
		return this->religion;
	}

	const centesimal_int &get_literacy_rate() const
	{
		return this->literacy_rate;
	}

	const std::vector<technology *> &get_technologies() const
	{
		return this->technologies;
	}

	Q_INVOKABLE void add_technology(technology *technology)
	{
		this->technologies.push_back(technology);
	}

	Q_INVOKABLE void remove_technology(technology *technology)
	{
		std::erase(this->technologies, technology);
	}

	const country_map<diplomacy_state> &get_diplomacy_states() const
	{
		return this->diplomacy_states;
	}

	const country_map<const consulate *> &get_consulates() const
	{
		return this->consulates;
	}

private:
	const metternich::country *country = nullptr;
	metternich::religion *religion = nullptr;
	centesimal_int literacy_rate;
	std::vector<technology *> technologies;
	country_map<diplomacy_state> diplomacy_states;
	country_map<const consulate *> consulates;
};

}
