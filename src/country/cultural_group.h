#pragma once

#include "country/culture_base.h"
#include "database/data_type.h"

namespace metternich {

enum class cultural_group_rank;

class cultural_group final : public culture_base, public data_type<cultural_group>
{
	Q_OBJECT

	Q_PROPERTY(metternich::cultural_group_rank rank MEMBER rank READ get_rank)

public:
	static constexpr const char class_identifier[] = "cultural_group";
	static constexpr const char property_class_identifier[] = "metternich::cultural_group*";
	static constexpr const char database_folder[] = "cultural_groups";
	static constexpr bool history_enabled = true;

	explicit cultural_group(const std::string &identifier);

	virtual void check() const override;

	cultural_group_rank get_rank() const
	{
		return this->rank;
	}

	const cultural_group *get_upper_group() const
	{
		return culture_base::get_group();
	}

private:
	cultural_group_rank rank;
};

}
