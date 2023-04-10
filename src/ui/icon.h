#pragma once

#include "database/data_type.h"
#include "ui/icon_base.h"

namespace metternich {

class icon final : public icon_base, public data_type<icon>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "icon";
	static constexpr const char property_class_identifier[] = "metternich::icon*";
	static constexpr const char database_folder[] = "icons";

	explicit icon(const std::string &identifier) : icon_base(identifier)
	{
	}

	virtual void initialize() override;
};

}
