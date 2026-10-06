#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class item_type;
class spell;

template <typename scope_type>
class and_condition;

//a starting package for a character
class character_package final : public named_data_entry, public data_type<character_package>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "character_package";
	static constexpr const char property_class_identifier[] = "metternich::character_package*";
	static constexpr const char database_folder[] = "character_packages";

	explicit character_package(const std::string &identifier);
	~character_package();

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const and_condition<character> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::vector<const item_type *> &get_items() const
	{
		return this->items;
	}

	const std::vector<const spell *> &get_spells() const
	{
		return this->spells;
	}

signals:
	void changed();

private:
	std::unique_ptr<const and_condition<character>> conditions;
	std::vector<const item_type *> items;
	std::vector<const spell *> spells;
};

}
