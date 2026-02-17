#pragma once

#include "database/data_entry_container.h"
#include "database/data_entry_history.h"
#include "domain/domain_container.h"
#include "domain/law_group_container.h"
#include "economy/commodity_container.h"
#include "util/centesimal_int.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("culture/culture.h")
Q_MOC_INCLUDE("domain/domain_tier.h")
Q_MOC_INCLUDE("domain/government_type.h")
Q_MOC_INCLUDE("religion/religion.h")

namespace metternich {

class character;
class consulate;
class culture;
class domain;
class government_type;
class law;
class office;
class religion;
class subject_type;
class technology;
enum class diplomacy_state;
enum class domain_tier;

class domain_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::domain_tier tier MEMBER tier)
	Q_PROPERTY(const metternich::culture* culture MEMBER culture READ get_culture)
	Q_PROPERTY(const metternich::religion* religion MEMBER religion READ get_religion)
	Q_PROPERTY(const metternich::government_type* government_type MEMBER government_type READ get_government_type)
	Q_PROPERTY(const metternich::domain* owner MEMBER owner READ get_owner)
	Q_PROPERTY(archimedes::centesimal_int literacy_rate MEMBER literacy_rate READ get_literacy_rate)
	Q_PROPERTY(std::vector<const metternich::technology *> technologies READ get_technologies)
	Q_PROPERTY(int wealth MEMBER wealth READ get_wealth)

public:
	explicit domain_history(const metternich::domain *domain);

	virtual void process_gsml_property(const gsml_property &property, const QDate &date) override;
	virtual void process_gsml_scope(const gsml_data &scope, const QDate &date) override;

	domain_tier get_tier() const
	{
		return this->tier;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
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

	const std::map<QDate, const character *> &get_historical_rulers() const
	{
		return this->historical_rulers;
	}

	const std::map<QDate, const character *> &get_historical_monarchs() const
	{
		return this->historical_monarchs;
	}

	domain_map<QDate> get_inserted_domain_ruler_histories(const QDate &max_date) const;

	const metternich::domain *get_owner() const
	{
		return this->owner;
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

	const domain_map<diplomacy_state> &get_diplomacy_states() const
	{
		return this->diplomacy_states;
	}

	diplomacy_state get_diplomacy_state(const metternich::domain *other_domain) const;

	const domain_map<const consulate *> &get_consulates() const
	{
		return this->consulates;
	}

private:
	const metternich::domain *domain = nullptr;
	domain_tier tier{};
	const metternich::culture *culture = nullptr;
	const metternich::religion *religion = nullptr;
	const metternich::government_type *government_type = nullptr;
	data_entry_map<office, const character *> office_holders;
	std::map<QDate, const character *> historical_rulers;
	std::map<QDate, const character *> historical_monarchs; //used for regnal numbers
	domain_map<QDate> inserted_domain_ruler_histories;
	const metternich::domain *owner = nullptr;
	const metternich::subject_type *subject_type = nullptr;
	centesimal_int literacy_rate;
	std::vector<const technology *> technologies;
	law_group_map<const law *> laws;
	int wealth = 0;
	commodity_map<int> commodities;
	domain_map<diplomacy_state> diplomacy_states;
	domain_map<const consulate *> consulates;
};

}
