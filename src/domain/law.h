#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "economy/commodity_container.h"

Q_MOC_INCLUDE("domain/law_group.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class domain;
class law_group;
class technology;
enum class succession_type;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class law final : public named_data_entry, public data_type<law>
{
	Q_OBJECT

	Q_PROPERTY(metternich::law_group * group MEMBER group NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(metternich::succession_type succession_type MEMBER succession_type READ get_succession_type NOTIFY changed)
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

	succession_type get_succession_type() const
	{
		return this->succession_type;
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

	const and_condition<domain> *get_conditions() const
	{
		return this->conditions.get();
	}

	const modifier<const domain> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(const metternich::domain *domain) const;

signals:
	void changed();

private:
	law_group *group = nullptr;
	const icon *icon = nullptr;
	metternich::succession_type succession_type {};
	technology *required_technology = nullptr;
	commodity_map<int> commodity_costs;
	std::unique_ptr<const and_condition<domain>> conditions;
	std::unique_ptr<const modifier<const domain>> modifier;
};

}
