#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class item_slot final : public named_data_entry, public data_type<item_slot>
{
	Q_OBJECT

	Q_PROPERTY(bool weapon MEMBER weapon READ is_weapon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "item_slot";
	static constexpr const char property_class_identifier[] = "metternich::item_slot*";
	static constexpr const char database_folder[] = "item_slots";

	explicit item_slot(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	bool is_weapon() const
	{
		return this->weapon;
	}

signals:
	void changed();

private:
	bool weapon = false;
};

}
