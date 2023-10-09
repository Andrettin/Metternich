#pragma once

#include "database/enum_data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class country;
enum class country_tier;

template <typename scope_type>
class modifier;

class country_tier_data final : public named_data_entry, public enum_data_type<country_tier_data, country_tier>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "country_tier";
	static constexpr const char property_class_identifier[] = "metternich::country_tier_data*";
	static constexpr const char database_folder[] = "country_tiers";

	explicit country_tier_data(const std::string &identifier);
	~country_tier_data();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const icon *get_icon() const
	{
		return this->icon;
	}

	const modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(metternich::country *country) const;

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	std::unique_ptr<const modifier<const country>> modifier;
};

}
