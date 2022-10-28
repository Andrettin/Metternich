#pragma once

#include "country/culture_base.h"
#include "database/data_type.h"

namespace metternich {

class cultural_group final : public culture_base, public data_type<cultural_group>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "cultural_group";
	static constexpr const char property_class_identifier[] = "metternich::cultural_group*";
	static constexpr const char database_folder[] = "cultural_groups";

	explicit cultural_group(const std::string &identifier) : culture_base(identifier)
	{
	}
};

}
