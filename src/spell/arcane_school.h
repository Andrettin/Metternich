#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class spell;

class arcane_school final : public named_data_entry, public data_type<arcane_school>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "arcane_school";
	static constexpr const char property_class_identifier[] = "metternich::arcane_school*";
	static constexpr const char database_folder[] = "arcane_schools";

	explicit arcane_school(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const std::vector<const spell *> &get_spells() const
	{
		return this->spells;
	}

	void add_spell(const spell *spell)
	{
		this->spells.push_back(spell);
	}

signals:
	void changed();

private:
	std::vector<const spell *> spells;
};

}
