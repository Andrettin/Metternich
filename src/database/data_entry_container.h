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

class cultural_group;
class idea_slot;
class office;
class technology_category;
class technology_subcategory;

extern template struct data_entry_compare<cultural_group>;
extern template struct data_entry_compare<game_rule>;
extern template struct data_entry_compare<idea_slot>;
extern template struct data_entry_compare<named_data_entry>;
extern template struct data_entry_compare<office>;
extern template struct data_entry_compare<technology_category>;
extern template struct data_entry_compare<technology_subcategory>;

}
