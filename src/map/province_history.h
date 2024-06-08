#pragma once

#include "database/data_entry_history.h"
#include "population/population_group_map.h"
#include "util/fractional_int.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("country/religion.h")

namespace metternich {

class country;
class culture;
class province;
class religion;

class province_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* owner MEMBER owner)
	Q_PROPERTY(metternich::culture* culture MEMBER culture)
	Q_PROPERTY(metternich::religion* religion MEMBER religion)
	Q_PROPERTY(int population READ get_population WRITE set_population)
	Q_PROPERTY(archimedes::centesimal_int literacy_rate MEMBER literacy_rate READ get_literacy_rate)

public:
	explicit province_history(const province *province) : province(province)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const country *get_owner() const
	{
		return this->owner;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::religion *get_religion() const
	{
		return this->religion;
	}

	int get_population() const
	{
		static const population_group_key group_key;

		return this->get_group_population(group_key);
	}

	void set_population(const int population)
	{
		static const population_group_key group_key;

		this->set_group_population(group_key, population);
	}

	const population_group_map<int> &get_population_groups() const
	{
		return this->population_groups;
	}

	int get_group_population(const population_group_key &group_key) const
	{
		const auto find_iterator = this->population_groups.find(group_key);
		if (find_iterator != this->population_groups.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_group_population(const population_group_key &group_key, const int population)
	{
		if (population == 0) {
			this->population_groups.erase(group_key);
		} else {
			this->population_groups[group_key] = population;
		}
	}

	void initialize_population();
	void distribute_population();

	const centesimal_int &get_literacy_rate() const
	{
		return this->literacy_rate;
	}

	void set_literacy_rate(const centesimal_int &literacy_rate)
	{
		this->literacy_rate = literacy_rate;
	}

private:
	const metternich::province *province = nullptr;
	country *owner = nullptr;
	metternich::culture *culture = nullptr;
	metternich::religion *religion = nullptr;
	population_group_map<int> population_groups;
	centesimal_int literacy_rate;
};

}
