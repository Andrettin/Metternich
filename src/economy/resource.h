#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class commodity;
class technology;

//resources are present on tiles, allowing the tile to produce a given commodity
//multiple resources can produce the same commodity
class resource final : public named_data_entry, public data_type<resource>
{
	Q_OBJECT

	Q_PROPERTY(metternich::commodity* commodity MEMBER commodity)
	Q_PROPERTY(std::filesystem::path icon_filepath MEMBER icon_filepath WRITE set_icon_filepath)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology)
	Q_PROPERTY(metternich::technology* discovery_technology MEMBER discovery_technology)

public:
	static constexpr const char class_identifier[] = "resource";
	static constexpr const char property_class_identifier[] = "metternich::resource*";
	static constexpr const char database_folder[] = "resources";

	explicit resource(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const metternich::commodity *get_commodity() const
	{
		return this->commodity;
	}

	const std::filesystem::path &get_icon_filepath() const
	{
		return this->icon_filepath;
	}

	void set_icon_filepath(const std::filesystem::path &filepath);

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const technology *get_discovery_technology() const
	{
		return this->discovery_technology;
	}

private:
	metternich::commodity *commodity = nullptr;
	std::filesystem::path icon_filepath;
	technology *required_technology = nullptr; //technology which is required to see the resource on the tile
	technology *discovery_technology = nullptr; //technology which is obtained when exploring this resource tile
};

}
