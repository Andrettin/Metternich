#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/dice.h"

Q_MOC_INCLUDE("item/item_class.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class character;
class icon;
class item_class;
class item_slot;

template <typename scope_type>
class modifier;

class item_type final : public named_data_entry, public data_type<item_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::item_class* item_class MEMBER item_class READ get_item_class NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(archimedes::dice damage_dice MEMBER damage_dice READ get_damage_dice NOTIFY changed)
	Q_PROPERTY(bool two_handed MEMBER two_handed READ is_two_handed NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "item_type";
	static constexpr const char property_class_identifier[] = "metternich::item_type*";
	static constexpr const char database_folder[] = "item_types";

	static std::vector<const item_type *> get_weapon_types()
	{
		std::vector<const item_type *> weapon_types;

		for (const item_type *item_type : item_type::get_all()) {
			if (!item_type->is_weapon()) {
				continue;
			}

			
			weapon_types.push_back(item_type);
		}

		return weapon_types;
	}

	explicit item_type(const std::string &identifier);
	~item_type();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::item_class *get_item_class() const
	{
		return this->item_class;
	}

	const item_slot *get_slot() const;
	bool is_weapon() const;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	int get_price() const
	{
		return this->price;
	}

	const dice &get_damage_dice() const
	{
		return this->damage_dice;
	}

	bool is_two_handed() const
	{
		return this->two_handed;
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

signals:
	void changed();

private:
	const metternich::item_class *item_class = nullptr;
	const metternich::icon *icon = nullptr;
	int price = 0;
	dice damage_dice;
	bool two_handed = false;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
};

}
