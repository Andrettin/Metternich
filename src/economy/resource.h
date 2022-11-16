#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class commodity;
class icon;
class technology;
class terrain_type;

//resources are present on tiles, allowing the tile to produce a given commodity
//multiple resources can produce the same commodity
class resource final : public named_data_entry, public data_type<resource>
{
	Q_OBJECT

	Q_PROPERTY(metternich::commodity* commodity MEMBER commodity NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* discovery_technology MEMBER discovery_technology NOTIFY changed)
	Q_PROPERTY(bool coastal MEMBER coastal READ is_coastal NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "resource";
	static constexpr const char property_class_identifier[] = "metternich::resource*";
	static constexpr const char database_folder[] = "resources";

	explicit resource(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::commodity *get_commodity() const
	{
		return this->commodity;
	}

	const metternich::icon *get_icon() const;

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const technology *get_discovery_technology() const
	{
		return this->discovery_technology;
	}

	bool is_coastal() const
	{
		return this->coastal;
	}

	const std::vector<const terrain_type *> &get_terrain_types() const
	{
		return this->terrain_types;
	}

signals:
	void changed();

private:
	metternich::commodity *commodity = nullptr;
	metternich::icon *icon = nullptr;
	technology *required_technology = nullptr; //technology which is required to see the resource on the tile
	technology *discovery_technology = nullptr; //technology which is obtained when exploring this resource tile
	bool coastal = false;
	std::vector<const terrain_type *> terrain_types;
};

}
