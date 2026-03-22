#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("item/item_slot.h")

namespace metternich {

class item_slot;

class item_class final : public named_data_entry, public data_type<item_class>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::item_slot* slot MEMBER slot READ get_slot NOTIFY changed)
	Q_PROPERTY(bool consumable MEMBER consumable READ is_consumable NOTIFY changed)
	Q_PROPERTY(QString consume_verb READ get_consume_verb_qstring NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "item_class";
	static constexpr const char property_class_identifier[] = "metternich::item_class*";
	static constexpr const char database_folder[] = "item_classes";

	explicit item_class(const std::string &identifier);
	~item_class();

	virtual void check() const override;

	const item_slot *get_slot() const
	{
		return this->slot;
	}

	bool is_weapon() const;

	bool is_consumable() const
	{
		return this->consumable;
	}

	const std::string &get_consume_verb() const
	{
		return this->consume_verb;
	}

	Q_INVOKABLE void set_consume_verb(const std::string &consume_verb)
	{
		this->consume_verb = consume_verb;
	}

	QString get_consume_verb_qstring() const
	{
		return QString::fromStdString(this->get_consume_verb());
	}

signals:
	void changed();

private:
	const item_slot *slot = nullptr;
	bool consumable = false;
	std::string consume_verb = "consume";
};

}
