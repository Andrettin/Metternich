#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class military_unit_class;
class cultural_group;
class culture;
class icon;
class technology;

class military_unit_type final : public named_data_entry, public data_type<military_unit_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::military_unit_class* unit_class MEMBER unit_class NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "military_unit_type";
	static constexpr const char property_class_identifier[] = "metternich::military_unit_type*";
	static constexpr const char database_folder[] = "military_unit_types";

public:
	explicit military_unit_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const military_unit_class *get_unit_class() const
	{
		return this->unit_class;
	}

	const metternich::culture *get_culture() const
	{
		return this->culture;
	}

	const metternich::cultural_group *get_cultural_group() const
	{
		return this->cultural_group;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

signals:
	void changed();

private:
	military_unit_class *unit_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::icon *icon = nullptr;
	technology *required_technology = nullptr;
};

}