#pragma once

#include "country/idea.h"
#include "database/data_type.h"

namespace metternich {

class research_organization final : public idea, public data_type<research_organization>
{
	Q_OBJECT

	Q_PROPERTY(int skill MEMBER skill READ get_skill NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "research_organization";
	static constexpr const char property_class_identifier[] = "metternich::research_organization*";
	static constexpr const char database_folder[] = "research_organizations";

	explicit research_organization(const std::string &identifier);
	~research_organization();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

	virtual idea_type get_idea_type() const override;

	virtual int get_skill() const override
	{
		return this->skill;
	}

signals:
	void changed();

private:
	int skill = 0;
};

}
