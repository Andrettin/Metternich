#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class mythic_path final : public named_data_entry, public data_type<mythic_path>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "mythic_path";
	static constexpr const char property_class_identifier[] = "metternich::mythic_path*";
	static constexpr const char database_folder[] = "mythic_paths";

	explicit mythic_path(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

signals:
	void changed();

private:
};

}
