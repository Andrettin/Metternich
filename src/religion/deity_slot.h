#pragma once

#include "database/data_type.h"
#include "domain/idea_slot.h"

namespace metternich {

class deity_slot final : public idea_slot, public data_type<deity_slot>
{
	Q_OBJECT

	Q_PROPERTY(bool major MEMBER major READ is_major NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "deity_slot";
	static constexpr const char property_class_identifier[] = "metternich::deity_slot*";
	static constexpr const char database_folder[] = "deity_slots";

public:
	explicit deity_slot(const std::string &identifier);
	~deity_slot();

	virtual idea_type get_idea_type() const override;

	bool is_major() const
	{
		return this->major;
	}

signals:
	void changed();

private:
	bool major = false;
};

}
