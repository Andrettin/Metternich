#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/dice.h"

Q_MOC_INCLUDE("item/item_class.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class character;
class enchantment;
class icon;
class item_class;
class item_material;
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
	Q_PROPERTY(bool stackable MEMBER stackable READ is_stackable NOTIFY changed)

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

	static void process_name_scope(data_entry_map<item_material, data_entry_map<enchantment, std::vector<std::string>>> &names, const gsml_data &scope);
	static void process_name_scope(data_entry_map<enchantment, std::vector<std::string>> &names, const gsml_data &scope);
	static void process_name_scope(std::vector<std::string> &names, const gsml_data &scope);

	explicit item_type(const std::string &identifier);
	~item_type();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
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

	bool is_stackable() const
	{
		return this->stackable;
	}

	const metternich::modifier<const character> *get_modifier() const
	{
		return this->modifier.get();
	}

	const std::vector<std::string> &get_names(const item_material *material, const enchantment *enchantment) const
	{
		const auto material_find_iterator = this->names.find(material);
		if (material_find_iterator != this->names.end()) {
			const auto enchantment_find_iterator = material_find_iterator->second.find(enchantment);
			if (enchantment_find_iterator != material_find_iterator->second.end()) {
				return enchantment_find_iterator->second;
			}
		}

		static const std::vector<std::string> empty_vector;
		return empty_vector;
	}

signals:
	void changed();

private:
	const metternich::item_class *item_class = nullptr;
	const metternich::icon *icon = nullptr;
	int price = 0;
	dice damage_dice;
	bool two_handed = false;
	bool stackable = false;
	std::unique_ptr<const metternich::modifier<const character>> modifier;
	data_entry_map<item_material, data_entry_map<enchantment, std::vector<std::string>>> names;
};

}
