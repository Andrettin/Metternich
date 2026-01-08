#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class skill;

class skill_group final : public named_data_entry, public data_type<skill_group>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "skill_group";
	static constexpr const char property_class_identifier[] = "metternich::skill_group*";
	static constexpr const char database_folder[] = "skill_groups";

	explicit skill_group(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const std::vector<const skill *> &get_skills() const
	{
		return this->skills;
	}

	void add_skill(const skill *skill)
	{
		this->skills.push_back(skill);
	}

signals:
	void changed();

private:
	std::vector<const skill *> skills;
};

}
