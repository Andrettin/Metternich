#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

namespace metternich {

class province;
class region_history;

class region final : public named_data_entry, public data_type<region>
{
	Q_OBJECT

	Q_PROPERTY(bool ocean MEMBER ocean READ is_ocean)
	Q_PROPERTY(std::vector<metternich::region *> superregions READ get_superregions)

public:
	static constexpr const char class_identifier[] = "region";
	static constexpr const char property_class_identifier[] = "metternich::region*";
	static constexpr const char database_folder[] = "regions";
	static constexpr bool history_enabled = true;

public:
	explicit region(const std::string &identifier);
	~region();

	virtual void initialize() override;
	virtual data_entry_history *get_history_base() override;

	region_history *get_history() const
	{
		return this->history.get();
	}

	virtual void reset_history() override;

	bool is_ocean() const
	{
		return this->ocean;
	}

	const std::vector<province *> &get_provinces() const
	{
		return this->provinces;
	}

	void add_province(province *province);

	void remove_province(province *province)
	{
		std::erase(this->provinces, province);
	}

	const std::vector<region *> &get_superregions() const
	{
		return this->superregions;
	}

	Q_INVOKABLE void add_superregion(region *superregion)
	{
		this->superregions.push_back(superregion);
		superregion->subregions.push_back(this);
	}

	Q_INVOKABLE void remove_superregion(region *superregion)
	{
		std::erase(this->superregions, superregion);
		std::erase(superregion->subregions, this);
	}

	bool is_part_of(const region *other_region) const;

private:
	bool ocean = false;
	std::vector<province *> provinces; //provinces located in the region
	std::vector<region *> subregions; //subregions of this region
	std::vector<region *> superregions; //regions for which this region is a subregion
	qunique_ptr<region_history> history;
};

}
