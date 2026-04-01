#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

namespace metternich {

class enchantment;
class item;
class item_type;
class site;
class spell;

class item_creation_type final : public named_data_entry, public data_type<item_creation_type>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "item_creation_type";
	static constexpr const char property_class_identifier[] = "metternich::item_creation_type*";
	static constexpr const char database_folder[] = "item_creation_types";

	explicit item_creation_type(const std::string &identifier);
	~item_creation_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	void add_enchantment(const enchantment *enchantment)
	{
		this->enchantments.push_back(enchantment);
	}

	qunique_ptr<item> create_item(const site *creation_site) const;

	bool can_create_item_type(const item_type *item_type) const;

signals:
	void changed();

private:
	std::vector<const item_creation_type *> item_creation_subtypes;
	std::vector<const item_type *> item_types;
	std::vector<const enchantment *> enchantments;
	std::vector<const spell *> spells;
};

}
