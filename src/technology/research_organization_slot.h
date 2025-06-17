#pragma once

#include "country/idea_slot.h"
#include "database/data_type.h"

namespace metternich {

class research_organization_slot final : public idea_slot, public data_type<research_organization_slot>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "research_organization_slot";
	static constexpr const char property_class_identifier[] = "metternich::research_organization_slot*";
	static constexpr const char database_folder[] = "research_organization_slots";

public:
	explicit research_organization_slot(const std::string &identifier);
	~research_organization_slot();

	virtual idea_type get_idea_type() const override;

signals:
	void changed();
};

}
