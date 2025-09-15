#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class character;
class item_class;
class item_type;
enum class affix_type;

template <typename scope_type>
class modifier;

class enchantment final : public named_data_entry, public data_type<enchantment>
{
	Q_OBJECT

	Q_PROPERTY(metternich::affix_type affix_type MEMBER affix_type READ get_affix_type NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "enchantment";
	static constexpr const char property_class_identifier[] = "metternich::enchantment*";
	static constexpr const char database_folder[] = "enchantments";

	explicit enchantment(const std::string &identifier);
	~enchantment();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	metternich::affix_type get_affix_type() const
	{
		return this->affix_type;
	}

	const data_entry_set<item_class> &get_item_classes() const
	{
		return this->item_classes;
	}

	const data_entry_set<item_type> &get_item_types() const
	{
		return this->item_types;
	}

	const std::vector<const enchantment *> &get_subenchantments() const
	{
		return this->subenchantments;
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

signals:
	void changed();

private:
	metternich::affix_type affix_type{};
	data_entry_set<item_class> item_classes;
	data_entry_set<item_type> item_types;
	std::vector<const enchantment *> subenchantments;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
};

}
