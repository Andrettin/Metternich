#pragma once

namespace metternich {

class scripted_character_modifier;
class scripted_modifier;
class scripted_province_modifier;
class scripted_site_modifier;

struct scripted_modifier_compare final
{
	bool operator()(const scripted_modifier *lhs, const scripted_modifier *rhs) const;
};

template <typename T>
using scripted_character_modifier_map = std::map<const scripted_character_modifier *, T, scripted_modifier_compare>;

template <typename T>
using scripted_province_modifier_map = std::map<const scripted_province_modifier *, T, scripted_modifier_compare>;

template <typename T>
using scripted_site_modifier_map = std::map<const scripted_site_modifier *, T, scripted_modifier_compare>;

}
