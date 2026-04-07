#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class character_attribute;
class domain_attribute;
class skill;

template <typename scope_type>
class and_condition;

class office final : public named_data_entry, public data_type<office>
{
	Q_OBJECT

	Q_PROPERTY(bool ruler READ is_ruler CONSTANT)
	Q_PROPERTY(bool heir READ is_heir CONSTANT)
	Q_PROPERTY(bool minister MEMBER minister READ is_minister NOTIFY changed)
	Q_PROPERTY(bool appointable READ is_appointable CONSTANT)
	Q_PROPERTY(const metternich::domain_attribute* domain_attribute MEMBER domain_attribute READ get_domain_attribute NOTIFY changed)
	Q_PROPERTY(bool half_domain_attribute_bonus MEMBER half_domain_attribute_bonus READ gives_half_domain_attribute_bonus NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "office";
	static constexpr const char property_class_identifier[] = "metternich::office*";
	static constexpr const char database_folder[] = "offices";

	explicit office(const std::string &identifier);
	~office();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	bool is_ruler() const;
	bool is_heir() const;

	bool is_minister() const
	{
		return this->minister;
	}

	bool is_appointable() const
	{
		return !this->is_ruler() && !this->is_heir();
	}

	const domain_attribute *get_domain_attribute() const
	{
		return this->domain_attribute;
	}

	bool gives_half_domain_attribute_bonus() const
	{
		return this->half_domain_attribute_bonus;
	}

	const std::vector<const character_attribute *> &get_character_attributes() const
	{
		return this->character_attributes;
	}

	const std::vector<const skill *> &get_skills() const
	{
		return this->skills;
	}

	const and_condition<domain> *get_conditions() const
	{
		return this->conditions.get();
	}

	const and_condition<character> *get_holder_conditions() const
	{
		return this->holder_conditions.get();
	}

signals:
	void changed();

private:
	bool minister = false;
	const metternich::domain_attribute *domain_attribute = nullptr;
	bool half_domain_attribute_bonus = false;
	std::vector<const character_attribute *> character_attributes;
	std::vector<const skill *> skills;
	std::unique_ptr<const and_condition<domain>> conditions;
	std::unique_ptr<const and_condition<character>> holder_conditions;
};

}
