#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class character_trait;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class trait_type final : public named_data_entry, public data_type<trait_type>
{
	Q_OBJECT

	Q_PROPERTY(int max_traits MEMBER max_traits READ get_max_traits NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "trait_type";
	static constexpr const char property_class_identifier[] = "metternich::trait_type*";
	static constexpr const char database_folder[] = "trait_types";

	explicit trait_type(const std::string &identifier);
	~trait_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	int get_max_traits() const
	{
		return this->max_traits;
	}

	const and_condition<character> *get_gain_conditions() const
	{
		return this->gain_conditions.get();
	}

	const modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

	const std::vector<const character_trait *> &get_traits() const
	{
		return this->traits;
	}

	void add_trait(const character_trait *trait)
	{
		this->traits.push_back(trait);
	}

signals:
	void changed();

private:
	int max_traits = 0; //the maximum amount of traits of this type a character can acquire
	std::unique_ptr<const and_condition<character>> gain_conditions;
	std::unique_ptr<const modifier<const character>> modifier;
	std::vector<const character_trait *> traits;
};

}
