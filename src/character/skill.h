#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/dice.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;

class skill final : public named_data_entry, public data_type<skill>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(archimedes::dice check_dice MEMBER check_dice READ get_check_dice NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "skill";
	static constexpr const char property_class_identifier[] = "metternich::skill*";
	static constexpr const char database_folder[] = "skills";

	explicit skill(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const metternich::icon *get_icon() const;
	const dice &get_check_dice() const;
	std::string_view get_value_suffix() const;

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	dice check_dice;
};

}
