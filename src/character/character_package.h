#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

namespace metternich {

class character;
class item_type;
class spell;

template <typename scope_type>
class and_condition;

//a starting package for a character
class character_package final : public data_entry, public data_type<character_package>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "character_package";
	static constexpr const char property_class_identifier[] = "metternich::character_package*";
	static constexpr const char database_folder[] = "character_packages";

	explicit character_package(const std::string &identifier);
	~character_package();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;

	const std::variant<int64_t, dice> &get_wealth_variant() const
	{
		return this->wealth_variant;
	}

	const commodity_unit *get_wealth_unit() const
	{
		return this->wealth_unit;
	}

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
	std::variant<int64_t, dice> wealth_variant = 0;
	const commodity_unit *wealth_unit = nullptr;
	std::unique_ptr<const and_condition<character>> conditions;
	std::vector<const item_type *> items;
	std::vector<const spell *> spells;
};

}
