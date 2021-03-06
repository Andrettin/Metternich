#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

#include <string>

namespace metternich {

class law_group final : public data_entry, public data_type<law_group>
{
	Q_OBJECT

public:
	law_group(const std::string &identifier) : data_entry(identifier) {}

	static constexpr const char *class_identifier = "law_group";
	static constexpr const char *database_folder = "law_groups";
};

}
