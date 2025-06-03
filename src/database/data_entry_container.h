#pragma once

namespace archimedes {
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
class office;

extern template struct data_entry_compare<cultural_group>;
extern template struct data_entry_compare<named_data_entry>;
extern template struct data_entry_compare<office>;

}
