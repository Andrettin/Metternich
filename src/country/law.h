#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"

Q_MOC_INCLUDE("country/law_group.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class law_group;
class technology;

template <typename scope_type>
class condition;

template <typename scope_type>
class modifier;

class law final : public named_data_entry, public data_type<law>
{
	Q_OBJECT

	Q_PROPERTY(metternich::law_group * group MEMBER group NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(QVariantList commodity_costs READ get_commodity_costs_qvariant_list NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "law";
	static constexpr const char property_class_identifier[] = "metternich::law*";
	static constexpr const char database_folder[] = "laws";

	explicit law(const std::string &identifier);
	~law();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const law_group *get_group() const
	{
		return this->group;
	}

	const icon *get_icon() const
	{
		return this->icon;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const commodity_map<int> &get_commodity_costs() const
	{
		return this->commodity_costs;
	}

	QVariantList get_commodity_costs_qvariant_list() const;

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
	law_group *group = nullptr;
	const icon *icon = nullptr;
	technology *required_technology = nullptr;
	commodity_map<int> commodity_costs;
	std::unique_ptr<const condition<country>> conditions;
	std::unique_ptr<const modifier<const country>> modifier;
};

}
