#pragma once

#include "database/enum_data_type.h"
#include "database/named_data_entry.h"
#include "domain/domain_tier.h"

namespace metternich {

class domain;

template <typename scope_type>
class modifier;

class domain_tier_data final : public named_data_entry, public enum_data_type<domain_tier_data, domain_tier>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "domain_tier";
	static constexpr const char property_class_identifier[] = "metternich::domain_tier_data*";
	static constexpr const char database_folder[] = "domain_tiers";

	explicit domain_tier_data(const std::string &identifier);
	~domain_tier_data();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const icon *get_icon() const
	{
		return this->icon;
	}

	const modifier<const domain> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(metternich::domain *domain) const;

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	std::unique_ptr<const modifier<const domain>> modifier;
};

}
