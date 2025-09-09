#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class character_attribute;

template <typename scope_type>
class and_condition;

class office final : public named_data_entry, public data_type<office>
{
	Q_OBJECT

	Q_PROPERTY(bool ruler READ is_ruler CONSTANT)
	Q_PROPERTY(bool minister MEMBER minister READ is_minister NOTIFY changed)
	Q_PROPERTY(bool appointable READ is_appointable CONSTANT)
	Q_PROPERTY(const metternich::character_attribute* attribute MEMBER attribute READ get_attribute NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "office";
	static constexpr const char property_class_identifier[] = "metternich::office*";
	static constexpr const char database_folder[] = "offices";

	explicit office(const std::string &identifier);
	~office();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	bool is_ruler() const;

	bool is_minister() const
	{
		return this->minister;
	}

	bool is_appointable() const
	{
		return !this->is_ruler();
	}

	const character_attribute *get_attribute() const
	{
		return this->attribute;
	}

	const and_condition<country> *get_conditions() const
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
	const character_attribute *attribute = nullptr;
	std::unique_ptr<const and_condition<country>> conditions;
	std::unique_ptr<const and_condition<character>> holder_conditions;
};

}
