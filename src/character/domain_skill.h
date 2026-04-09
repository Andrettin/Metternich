#pragma once

#include "character/character_stat.h"
#include "database/data_type.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;

class domain_skill final : public character_stat, public data_type<domain_skill>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "domain_skill";
	static constexpr const char property_class_identifier[] = "metternich::domain_skill*";
	static constexpr const char database_folder[] = "domain_skills";

	explicit domain_skill(const std::string &identifier) : character_stat(identifier)
	{
	}

	virtual void check() const override;

	const metternich::icon *get_icon() const;

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
};

}
