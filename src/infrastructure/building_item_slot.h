#pragma once

#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("infrastructure/building_slot.h")
Q_MOC_INCLUDE("item/item.h")
Q_MOC_INCLUDE("item/item_creation_type.h")

namespace metternich {

class building_slot;
class item;
class item_creation_type;

class building_item_slot final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::item_creation_type* item_creation_type READ get_item_creation_type CONSTANT)
	Q_PROPERTY(const metternich::item* item READ get_item NOTIFY item_changed)
	Q_PROPERTY(const metternich::building_slot* building_slot READ get_building_slot CONSTANT)

public:
	explicit building_item_slot(const item_creation_type *item_creation_type, const building_slot *building_slot);
	explicit building_item_slot(const gsml_data &scope, const building_slot *building_slot);
	~building_item_slot();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	const item_creation_type *get_item_creation_type() const
	{
		return this->item_creation_type;
	}

	const item *get_item() const
	{
		return this->item.get();
	}

	void create_item();
	qunique_ptr<item> take_item();

	Q_INVOKABLE bool can_buy_item(const metternich::character *buyer);
	Q_INVOKABLE void buy_item(const metternich::character *buyer);

	const building_slot *get_building_slot() const
	{
		return this->building_slot;
	}

signals:
	void item_changed();

private:
	const metternich::item_creation_type *item_creation_type = nullptr;
	qunique_ptr<metternich::item> item;
	const metternich::building_slot *building_slot = nullptr;
};

}
