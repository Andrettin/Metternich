#pragma once

namespace metternich {

class transporter_type;

struct transporter_type_compare final
{
	bool operator()(const transporter_type *lhs, const transporter_type *rhs) const;
};

using transporter_type_set = std::set<const transporter_type *, transporter_type_compare>;

template <typename T>
using transporter_type_map = std::map<const transporter_type *, T, transporter_type_compare>;

}
