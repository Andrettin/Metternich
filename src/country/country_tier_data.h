#pragma once

#include "database/enum_data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

enum class country_tier;

class country_tier_data final : public named_data_entry, public enum_data_type<country_tier_data, country_tier>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "country_tier";
	static constexpr const char property_class_identifier[] = "metternich::country_tier_data*";
	static constexpr const char database_folder[] = "country_tiers";

	explicit country_tier_data(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	const icon *get_icon() const
	{
		return this->icon;
	}

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
};

}
