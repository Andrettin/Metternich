#pragma once

namespace metternich {

class scripted_modifier;

struct scripted_modifier_compare final
{
	bool operator()(const scripted_modifier *lhs, const scripted_modifier *rhs) const;
};

}
