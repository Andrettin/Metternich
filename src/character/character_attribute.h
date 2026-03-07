#pragma once

#include "character/character_attribute_base.h"
#include "database/data_type.h"

namespace metternich {

class skill;

class character_attribute final : public character_attribute_base, public data_type<character_attribute>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "character_attribute";
	static constexpr const char property_class_identifier[] = "metternich::character_attribute*";
	static constexpr const char database_folder[] = "character_attributes";

	explicit character_attribute(const std::string &identifier);
	~character_attribute();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const std::pair<int, int> &get_rating_range(const std::string &rating) const
	{
		const auto find_iterator = this->rating_ranges.find(rating);

		if (find_iterator != this->rating_ranges.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error(std::format("Invalid rating for attribute \"{}\": \"{}\".", this->get_identifier(), rating));
	}

	const std::vector<const skill *> &get_derived_skills() const
	{
		return this->derived_skills;
	}
	
	void add_derived_skill(const skill *skill)
	{
		this->derived_skills.push_back(skill);
	}

signals:
	void changed();

private:
	std::map<std::string, std::pair<int, int>> rating_ranges; //names for particular ranges
	std::vector<const skill *> derived_skills;
};

}
