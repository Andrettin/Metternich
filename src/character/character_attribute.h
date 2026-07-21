#pragma once

#include "character/character_stat.h"
#include "database/data_type.h"

namespace metternich {

class skill;
enum class character_attribute_type;

class character_attribute final : public character_stat, public data_type<character_attribute>
{
	Q_OBJECT

	Q_PROPERTY(metternich::character_attribute_type type MEMBER type READ get_type NOTIFY changed)
	Q_PROPERTY(metternich::character_attribute* base_attribute MEMBER base_attribute NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "character_attribute";
	static constexpr const char property_class_identifier[] = "metternich::character_attribute*";
	static constexpr const char database_folder[] = "character_attributes";

	static void initialize_all();

	explicit character_attribute(const std::string &identifier);
	~character_attribute();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

	character_attribute_type get_type() const
	{
		return this->type;
	}

	const character_attribute *get_base_attribute() const
	{
		return this->base_attribute;
	}

	bool is_subattribute() const
	{
		return this->get_base_attribute() != nullptr;
	}

	const std::pair<int, int> &get_rating_range(const std::string &rating) const
	{
		const auto find_iterator = this->rating_ranges.find(rating);

		if (find_iterator != this->rating_ranges.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error(std::format("Invalid rating for attribute \"{}\": \"{}\".", this->get_identifier(), rating));
	}

	const std::vector<const character_attribute *> &get_subattributes() const
	{
		return this->subattributes;
	}
	
	void add_subattribute(const character_attribute *attribute)
	{
		if (this->is_subattribute()) {
			throw std::runtime_error(std::format("Tried to add a subattribute to subattribute \"{}\". Only main attributes can have subattributes.", this->get_identifier()));
		}

		this->subattributes.push_back(attribute);
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
	character_attribute_type type{};
	character_attribute *base_attribute = nullptr;
	std::map<std::string, std::pair<int, int>> rating_ranges; //names for particular ranges
	std::vector<const character_attribute *> subattributes;
	std::vector<const skill *> derived_skills;
};

}
