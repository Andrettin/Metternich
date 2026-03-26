#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class spell;

class divine_domain final : public named_data_entry, public data_type<divine_domain>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "divine_domain";
	static constexpr const char property_class_identifier[] = "metternich::divine_domain*";
	static constexpr const char database_folder[] = "divine_domains";

	explicit divine_domain(const std::string &identifier) : named_data_entry(identifier)
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
