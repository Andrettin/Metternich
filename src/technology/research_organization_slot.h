#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

template <typename scope_type>
class and_condition;

class research_organization_slot final : public named_data_entry, public data_type<research_organization_slot>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "research_organization_slot";
	static constexpr const char property_class_identifier[] = "metternich::research_organization_slot*";
	static constexpr const char database_folder[] = "research_organization_slots";

public:
	explicit research_organization_slot(const std::string &identifier);
	~research_organization_slot();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	std::unique_ptr<const and_condition<country>> conditions;
};

}
