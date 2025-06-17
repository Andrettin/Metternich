#pragma once

#include "country/idea_trait.h"
#include "database/data_type.h"

namespace metternich {

class research_organization_trait final : public idea_trait, public data_type<research_organization_trait>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "research_organization_trait";
	static constexpr const char property_class_identifier[] = "metternich::research_organization_trait*";
	static constexpr const char database_folder[] = "traits/research_organization";

	explicit research_organization_trait(const std::string &identifier);
	~research_organization_trait();

	virtual idea_type get_idea_type() const override;

signals:
	void changed();
};

}
