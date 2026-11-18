#pragma once

namespace metternich {

class character;
class character_event;
class character_scripted_condition;
class character_scripted_effect;
class domain;
class domain_event;
class domain_scripted_condition;
class domain_scripted_effect;
class military_unit;
class military_unit_scripted_condition;
class military_unit_scripted_effect;
class population_unit;
class population_unit_scripted_condition;
class population_unit_scripted_effect;
class province;
class province_event;
class province_scripted_condition;
class province_scripted_effect;
class scripted_character_modifier;
class scripted_domain_modifier;
class scripted_province_modifier;
class scripted_site_modifier;
class site;
class site_event;
class site_scripted_condition;
class site_scripted_effect;

template <typename scope_type>
struct scope_traits;

template <>
struct scope_traits<character> final
{
	using event_type = character_event;
	using scripted_condition_type = character_scripted_condition;
	using scripted_effect_type = character_scripted_effect;
	using scripted_modifier_type = scripted_character_modifier;

	static constexpr std::string_view delayed_effects_tag = "character_delayed_effects";
};

template <>
struct scope_traits<domain> final
{
	using event_type = domain_event;
	using scripted_condition_type = domain_scripted_condition;
	using scripted_effect_type = domain_scripted_effect;
	using scripted_modifier_type = scripted_domain_modifier;

	static constexpr std::string_view delayed_effects_tag = "domain_delayed_effects";
};

template <>
struct scope_traits<military_unit> final
{
	using scripted_condition_type = military_unit_scripted_condition;
	using scripted_effect_type = military_unit_scripted_effect;
};

template <>
struct scope_traits<population_unit> final
{
	using scripted_condition_type = population_unit_scripted_condition;
	using scripted_effect_type = population_unit_scripted_effect;
};

template <>
struct scope_traits<province> final
{
	using event_type = province_event;
	using scripted_condition_type = province_scripted_condition;
	using scripted_effect_type = province_scripted_effect;
	using scripted_modifier_type = scripted_province_modifier;

	static constexpr std::string_view delayed_effects_tag = "province_delayed_effects";
};

template <>
struct scope_traits<site> final
{
	using event_type = site_event;
	using scripted_condition_type = site_scripted_condition;
	using scripted_effect_type = site_scripted_effect;
	using scripted_modifier_type = scripted_site_modifier;

	static constexpr std::string_view delayed_effects_tag = "site_delayed_effects";
};

template <typename scope_type>
using scope_traits_t = scope_traits<std::remove_const_t<scope_type>>;

template <typename scope_type>
concept scope_has_events = requires { typename scope_traits_t<scope_type>::event_type; };

template <typename scope_type>
concept scope_has_scripted_modifiers = requires { typename scope_traits_t<scope_type>::scripted_modifier_type; };

}
