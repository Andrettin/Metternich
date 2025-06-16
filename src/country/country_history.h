#pragma once

#include "country/country_container.h"
#include "country/law_group_container.h"
#include "database/data_entry_container.h"
#include "database/data_entry_history.h"
#include "economy/commodity_container.h"
#include "util/fractional_int.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("country/government_type.h")
Q_MOC_INCLUDE("religion/religion.h")

namespace metternich {

class character;
class consulate;
class country;
class government_type;
class law;
class office;
class religion;
class subject_type;
class technology;
class tradition;
enum class country_tier;
enum class diplomacy_state;

class country_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::country_tier tier MEMBER tier)
	Q_PROPERTY(const metternich::religion* religion MEMBER religion READ get_religion)
	Q_PROPERTY(const metternich::government_type* government_type MEMBER government_type READ get_government_type)
	Q_PROPERTY(archimedes::centesimal_int literacy_rate MEMBER literacy_rate READ get_literacy_rate)
	Q_PROPERTY(std::vector<const metternich::technology *> technologies READ get_technologies)
	Q_PROPERTY(std::vector<const metternich::tradition *> traditions READ get_traditions)
	Q_PROPERTY(int wealth MEMBER wealth READ get_wealth)

public:
	explicit country_history(const metternich::country *country);

	virtual void process_gsml_scope(const gsml_data &scope) override;

	country_tier get_tier() const
	{
		return this->tier;
	}

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	const metternich::government_type *get_government_type() const
	{
		return this->government_type;
	}

	const data_entry_map<office, const character *> &get_office_holders() const
	{
		return this->office_holders;
	}

	const metternich::subject_type *get_subject_type() const
	{
		return this->subject_type;
	}

	const centesimal_int &get_literacy_rate() const
	{
		return this->literacy_rate;
	}

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

	const std::vector<const tradition *> &get_traditions() const
	{
		return this->traditions;
	}

	Q_INVOKABLE void add_tradition(const tradition *tradition)
	{
		this->traditions.push_back(tradition);
	}

	Q_INVOKABLE void remove_tradition(const tradition *tradition)
	{
		std::erase(this->traditions, tradition);
	}

	const law_group_map<const law *> &get_laws() const
	{
		return this->laws;
	}

	int get_wealth() const
	{
		return this->wealth;
	}

	const commodity_map<int> &get_commodities() const
	{
		return this->commodities;
	}

	const country_map<diplomacy_state> &get_diplomacy_states() const
	{
		return this->diplomacy_states;
	}

	diplomacy_state get_diplomacy_state(const metternich::country *other_country) const;

	const country_map<const consulate *> &get_consulates() const
	{
		return this->consulates;
	}

private:
	const metternich::country *country = nullptr;
	country_tier tier{};
	const metternich::religion *religion = nullptr;
	const metternich::government_type *government_type = nullptr;
	data_entry_map<office, const character *> office_holders;
	const metternich::subject_type *subject_type = nullptr;
	centesimal_int literacy_rate;
	std::vector<const technology *> technologies;
	std::vector<const tradition *> traditions;
	law_group_map<const law *> laws;
	int wealth = 0;
	commodity_map<int> commodities;
	country_map<diplomacy_state> diplomacy_states;
	country_map<const consulate *> consulates;
};

}
