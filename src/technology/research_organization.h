#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class country;
class portrait;
class research_organization_trait;
class technology;

template <typename scope_type>
class and_condition;

class research_organization final : public named_data_entry, public data_type<research_organization>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(int skill MEMBER skill READ get_skill NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "research_organization";
	static constexpr const char property_class_identifier[] = "metternich::research_organization*";
	static constexpr const char database_folder[] = "research_organizations";

	explicit research_organization(const std::string &identifier);
	~research_organization();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	int get_skill() const
	{
		return this->skill;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::vector<const research_organization_trait *> &get_traits() const
	{
		return this->traits;
	}

	std::string get_modifier_string(const country *country) const;

	Q_INVOKABLE QString get_modifier_qstring(const country *country) const
	{
		return QString::fromStdString(this->get_modifier_string(country));
	}

	void apply_modifier(const country *country, const int multiplier) const;
	void apply_trait_modifier(const research_organization_trait *trait, const country *country, const int multiplier) const;

signals:
	void changed();

private:
	const metternich::portrait *portrait = nullptr;
	int skill = 0;
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	std::unique_ptr<const and_condition<country>> conditions;
	std::vector<const research_organization_trait *> traits;
};

}
