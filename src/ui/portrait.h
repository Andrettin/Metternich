#pragma once

#include "database/data_type.h"
#include "ui/icon_base.h"

namespace metternich {

class portrait final : public icon_base, public data_type<portrait>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "portrait";
	static constexpr const char property_class_identifier[] = "metternich::portrait*";
	static constexpr const char database_folder[] = "portraits";

	explicit portrait(const std::string &identifier) : icon_base(identifier)
	{
	}

	virtual void initialize() override;
};

}
