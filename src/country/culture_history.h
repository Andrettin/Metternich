#pragma once

#include "database/data_entry_history.h"

namespace metternich {

class country;
class province;
class region;
class technology;

class culture_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(std::vector<metternich::technology *> technologies READ get_technologies)
	Q_PROPERTY(std::vector<const metternich::province *> explored_provinces READ get_explored_provinces)
	Q_PROPERTY(std::vector<const metternich::region *> explored_regions READ get_explored_regions)

public:
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

	const std::vector<const province *> &get_explored_provinces() const
	{
		return this->explored_provinces;
	}

	Q_INVOKABLE void add_explored_province(const province *province)
	{
		this->explored_provinces.push_back(province);
	}

	Q_INVOKABLE void remove_explored_province(const province *province)
	{
		std::erase(this->explored_provinces, province);
	}

	const std::vector<const region *> &get_explored_regions() const
	{
		return this->explored_regions;
	}

	Q_INVOKABLE void add_explored_region(const region *region)
	{
		this->explored_regions.push_back(region);
	}

	Q_INVOKABLE void remove_explored_region(const region *region)
	{
		std::erase(this->explored_regions, region);
	}

	void apply_to_country(const country *country) const;

private:
	std::vector<technology *> technologies;
	std::vector<const province *> explored_provinces;
	std::vector<const region *> explored_regions;
};

}
