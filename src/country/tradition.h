#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("country/tradition_group.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class icon;
class portrait;
class technology;
class tradition_group;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class tradition final : public named_data_entry, public data_type<tradition>
{
	Q_OBJECT

	Q_PROPERTY(metternich::tradition_group* group MEMBER group NOTIFY changed)
	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "tradition";
	static constexpr const char property_class_identifier[] = "metternich::tradition*";
	static constexpr const char database_folder[] = "traditions";

	explicit tradition(const std::string &identifier);
	~tradition();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const tradition_group *get_group() const
	{
		return this->group;
	}

	const portrait *get_portrait() const
	{
		return this->portrait;
	}

	const icon *get_icon() const
	{
		return this->icon;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const std::vector<const tradition *> get_prerequisites() const
	{
		return this->prerequisites;
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(const metternich::country *country) const;

signals:
	void changed();

private:
	tradition_group *group = nullptr;
	const portrait *portrait = nullptr;
	const icon *icon = nullptr;
	technology *required_technology = nullptr;
	std::vector<const tradition *> prerequisites;
	std::unique_ptr<const condition<country>> conditions;
	std::unique_ptr<const modifier<const country>> modifier;
};

}
