#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

template <typename scope_type>
class and_condition;

class deity_slot final : public named_data_entry, public data_type<deity_slot>
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

	virtual void process_gsml_scope(const gsml_data &scope) override;

	bool is_major() const
	{
		return this->major;
	}

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	bool major = false;
	std::unique_ptr<const and_condition<country>> conditions;
};

}
