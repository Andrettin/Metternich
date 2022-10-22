#pragma once

#include "country/country_container.h"
#include "database/data_entry_history.h"

namespace metternich {

class country;
enum class diplomacy_state;

class country_history final : public data_entry_history
{
	Q_OBJECT

public:
	explicit country_history(const metternich::country *country) : country(country)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const country_map<diplomacy_state> &get_diplomacy_states() const
	{
		return this->diplomacy_states;
	}

private:
	const metternich::country *country = nullptr;
	country_map<diplomacy_state> diplomacy_states;
};

}
