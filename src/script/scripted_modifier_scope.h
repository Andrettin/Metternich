#pragma once

#include "script/scope_traits.h"
#include "script/scripted_modifier_compare.h"

namespace metternich {

class character;
class character_game_data;
class domain;
class domain_game_data;
class province;
class province_game_data;
class site;
class site_game_data;

template <typename derived_type, scope_has_scripted_modifiers scope_type>
class scripted_modifier_scope
{
public:
	using scripted_modifier_type = scope_traits_t<scope_type>::scripted_modifier_type;

	template <typename T>
	using scripted_modifier_map = std::map<const scripted_modifier_type *, T, scripted_modifier_compare>;

	const scripted_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

protected:
	scripted_modifier_map<int> &get_scripted_modifiers()
	{
		return this->scripted_modifiers;
	}

public:
	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_modifier_type *modifier) const;
	[[nodiscard]] QCoro::Task<void> add_scripted_modifier(const scripted_modifier_type *modifier, const int duration);
	[[nodiscard]] QCoro::Task<void> remove_scripted_modifier(const scripted_modifier_type *modifier);
	[[nodiscard]] QCoro::Task<void> decrement_scripted_modifiers();

private:
	derived_type *get_derived()
	{
		return static_cast<derived_type *>(this);
	}

	scripted_modifier_map<int> scripted_modifiers;
};

extern template class scripted_modifier_scope<character_game_data, const character>;
extern template class scripted_modifier_scope<domain_game_data, const domain>;
extern template class scripted_modifier_scope<province_game_data, const province>;
extern template class scripted_modifier_scope<site_game_data, const site>;

}
