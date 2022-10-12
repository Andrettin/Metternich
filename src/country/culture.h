#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class cultural_group;

class culture final : public named_data_entry, public data_type<culture>
{
	Q_OBJECT

	Q_PROPERTY(metternich::cultural_group* group MEMBER group)

public:
	static constexpr const char class_identifier[] = "culture";
	static constexpr const char property_class_identifier[] = "metternich::culture*";
	static constexpr const char database_folder[] = "cultures";

	explicit culture(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const cultural_group *get_group() const
	{
		return this->group;
	}

private:
	cultural_group *group = nullptr;
};

}
