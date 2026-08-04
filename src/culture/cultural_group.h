#pragma once

#include "database/data_type.h"
#include "culture/culture_base.h"

namespace metternich {

class culture;
class domain;
enum class cultural_group_rank;

class cultural_group final : public culture_base, public data_type<cultural_group>
{
	Q_OBJECT

	Q_PROPERTY(metternich::cultural_group_rank rank MEMBER rank READ get_rank)
	Q_PROPERTY(metternich::domain* cultural_union MEMBER cultural_union NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "cultural_group";
	static constexpr const char property_class_identifier[] = "metternich::cultural_group*";
	static constexpr const char database_folder[] = "cultural_groups";
	static constexpr bool history_enabled = true;

	using culture_base::database_dependencies;

	explicit cultural_group(const std::string &identifier);

	virtual void initialize() override;
	virtual void check() const override;

	cultural_group_rank get_rank() const
	{
		return this->rank;
	}

	const cultural_group *get_upper_group() const
	{
		return culture_base::get_group();
	}

	cultural_group *get_upper_group()
	{
		return culture_base::get_group();
	}

	virtual void set_group(cultural_group *group) override
	{
		culture_base::set_group(group);

		for (const culture *culture : this->get_cultures()) {
			group->add_culture(culture);
		}
	}

	const domain *get_cultural_union() const
	{
		return this->cultural_union;
	}

	const std::vector<const culture *> &get_cultures() const
	{
		return this->cultures;
	}

	void add_culture(const culture *culture);

private:
	cultural_group_rank rank;
	domain *cultural_union = nullptr;
	std::vector<const culture *> cultures;
};

}
