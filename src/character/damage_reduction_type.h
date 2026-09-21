#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("item/item_material.h")

namespace metternich {

class item_material;

class damage_reduction_type final : public named_data_entry, public data_type<damage_reduction_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::item_material* vulnerability_material MEMBER vulnerability_material READ get_vulnerability_material NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "damage_reduction_type";
	static constexpr const char property_class_identifier[] = "metternich::damage_reduction_type*";
	static constexpr const char database_folder[] = "damage_reduction_types";

	explicit damage_reduction_type(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	const metternich::item_material *get_vulnerability_material() const
	{
		return this->vulnerability_material;
	}

	bool has_vulnerability() const
	{
		return this->get_vulnerability_material() != nullptr;
	}

signals:
	void changed();

private:
	const item_material *vulnerability_material = nullptr;
};

}
