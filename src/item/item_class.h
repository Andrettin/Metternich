#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("item/item_slot.h")

namespace metternich {

class item_slot;

class item_class final : public named_data_entry, public data_type<item_class>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::item_slot* slot MEMBER slot READ get_slot NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "item_class";
	static constexpr const char property_class_identifier[] = "metternich::item_class*";
	static constexpr const char database_folder[] = "item_classes";

	explicit item_class(const std::string &identifier);
	~item_class();

	const item_slot *get_slot() const
	{
		return this->slot;
	}

	bool is_weapon() const;

signals:
	void changed();

private:
	const item_slot *slot = nullptr;
};

}
