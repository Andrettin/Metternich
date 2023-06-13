#pragma once

namespace metternich {

class transporter_class;

struct transporter_class_compare final
{
	bool operator()(const transporter_class *lhs, const transporter_class *rhs) const;
};

using transporter_class_set = std::set<const transporter_class *, transporter_class_compare>;

template <typename T>
using transporter_class_map = std::map<const transporter_class *, T, transporter_class_compare>;

}
