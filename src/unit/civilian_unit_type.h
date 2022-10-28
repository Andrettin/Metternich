#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class civilian_unit_class;
class cultural_group;
class culture;
class icon;

class civilian_unit_type final : public named_data_entry, public data_type<civilian_unit_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::civilian_unit_class* unit_class MEMBER unit_class)
	Q_PROPERTY(metternich::culture* culture MEMBER culture)
	Q_PROPERTY(metternich::cultural_group* cultural_group MEMBER cultural_group)
	Q_PROPERTY(metternich::icon* icon MEMBER icon)

public:
	static constexpr const char *class_identifier = "civilian_unit_type";
	static constexpr const char property_class_identifier[] = "metternich::civilian_unit_type*";
	static constexpr const char *database_folder = "civilian_unit_types";

public:
	explicit civilian_unit_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const civilian_unit_class *get_unit_class() const
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

private:
	civilian_unit_class *unit_class = nullptr;
	metternich::culture *culture = nullptr;
	metternich::cultural_group *cultural_group = nullptr;
	metternich::icon *icon = nullptr;
};

}
