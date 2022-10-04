#pragma once

namespace metternich {

class province;

struct province_compare
{
	bool operator()(const province *province, const metternich::province *other_province) const;
};

using province_set = std::set<const province *, province_compare>;

template <typename T>
using province_map = std::map<const province *, T, province_compare>;

}
