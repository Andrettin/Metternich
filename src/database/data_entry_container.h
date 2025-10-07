#pragma once

namespace archimedes {
	class game_rule;
	class named_data_entry;
}

namespace metternich {

template <typename T>
struct data_entry_compare final
{
	bool operator()(const T *lhs, const T *rhs) const;
};

template <typename T>
using data_entry_set = std::set<const T *, data_entry_compare<T>>;

template <typename T, typename U>
using data_entry_map = std::map<const T *, U, data_entry_compare<T>>;

class character_attribute;
class character_class;
class civilian_unit_type;
class commodity_unit;
class cultural_group;
class dungeon_area;
class idea_slot;
class item_class;
class item_slot;
class item_type;
class monster_type;
class office;
class saving_throw_type;
class skill;
class species;
class status_effect;
class technology_category;
class technology_subcategory;
class trait;
class trait_type;

extern template struct data_entry_compare<character_attribute>;
extern template struct data_entry_compare<character_class>;
extern template struct data_entry_compare<civilian_unit_type>;
extern template struct data_entry_compare<commodity_unit>;
extern template struct data_entry_compare<cultural_group>;
extern template struct data_entry_compare<dungeon_area>;
extern template struct data_entry_compare<game_rule>;
extern template struct data_entry_compare<idea_slot>;
extern template struct data_entry_compare<item_class>;
extern template struct data_entry_compare<item_slot>;
extern template struct data_entry_compare<item_type>;
extern template struct data_entry_compare<monster_type>;
extern template struct data_entry_compare<named_data_entry>;
extern template struct data_entry_compare<office>;
extern template struct data_entry_compare<saving_throw_type>;
extern template struct data_entry_compare<skill>;
extern template struct data_entry_compare<species>;
extern template struct data_entry_compare<status_effect>;
extern template struct data_entry_compare<technology_category>;
extern template struct data_entry_compare<technology_subcategory>;
extern template struct data_entry_compare<trait>;
extern template struct data_entry_compare<trait_type>;

}
