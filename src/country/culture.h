#pragma once

#include "country/culture_base.h"
#include "database/data_type.h"

namespace metternich {

class cultural_group;

class culture final : public culture_base, public data_type<culture>
{
	Q_OBJECT

	Q_PROPERTY(metternich::cultural_group* group MEMBER group)

public:
	static constexpr const char class_identifier[] = "culture";
	static constexpr const char property_class_identifier[] = "metternich::culture*";
	static constexpr const char database_folder[] = "cultures";

	explicit culture(const std::string &identifier) : culture_base(identifier)
	{
	}

	virtual void check() const override;

	const cultural_group *get_group() const
	{
		return this->group;
	}

	const population_type *get_population_class_type(const population_class *population_class) const;

private:
	cultural_group *group = nullptr;
};

}
