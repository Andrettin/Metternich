#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;

class technology final : public named_data_entry, public data_type<technology>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon)

public:
	static constexpr const char class_identifier[] = "technology";
	static constexpr const char property_class_identifier[] = "metternich::technology*";
	static constexpr const char database_folder[] = "technologies";

	explicit technology(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

private:
	metternich::icon *icon = nullptr;
};

}