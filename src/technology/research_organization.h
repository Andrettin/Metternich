#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class portrait;
class research_organization;
class research_organization_trait;

class research_organization final : public named_data_entry, public data_type<research_organization>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(int skill MEMBER skill READ get_skill NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "research_organization";
	static constexpr const char property_class_identifier[] = "metternich::research_organization*";
	static constexpr const char database_folder[] = "research_organizations";

	explicit research_organization(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	int get_skill() const
	{
		return this->skill;
	}

	const std::vector<const research_organization_trait *> &get_traits() const
	{
		return this->traits;
	}

signals:
	void changed();

private:
	const metternich::portrait *portrait = nullptr;
	int skill = 0;
	std::vector<const research_organization_trait *> traits;
};

}
