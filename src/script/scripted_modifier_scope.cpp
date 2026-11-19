#include "metternich.h"

#include "script/scripted_modifier_scope.h"

#include "character/character_game_data.h"
#include "domain/domain_game_data.h"
#include "game/game.h"
#include "map/province_game_data.h"
#include "map/site_game_data.h"
#include "script/scripted_character_modifier.h"
#include "script/scripted_domain_modifier.h"
#include "script/scripted_province_modifier.h"
#include "script/scripted_site_modifier.h"
#include "script/scripted_modifier.h"
#include "util/map_util.h"

namespace metternich {

template <typename derived_type, scope_has_scripted_modifiers scope_type>
QVariantList scripted_modifier_scope<derived_type, scope_type>::get_scripted_modifiers_qvariant_list() const
{
	return archimedes::map::to_qvariant_list(this->get_scripted_modifiers());
}

template <typename derived_type, scope_has_scripted_modifiers scope_type>
bool scripted_modifier_scope<derived_type, scope_type>::has_scripted_modifier(const scripted_modifier_type *modifier) const
{
	return this->get_scripted_modifiers().contains(modifier);
}

template <typename derived_type, scope_has_scripted_modifiers scope_type>
QCoro::Task<void> scripted_modifier_scope<derived_type, scope_type>::add_scripted_modifier(const scripted_modifier_type *modifier, const int duration)
{
	if (!this->scripted_modifiers.contains(modifier) && modifier->get_modifier() != nullptr) {
		co_await this->get_derived()->apply_modifier(modifier->get_modifier(), 1);
	}

	this->scripted_modifiers[modifier] = std::max(this->scripted_modifiers[modifier], duration);

	if (game::get()->is_running()) {
		emit this->get_derived()->scripted_modifiers_changed();
	}
}

template <typename derived_type, scope_has_scripted_modifiers scope_type>
QCoro::Task<void> scripted_modifier_scope<derived_type, scope_type>::remove_scripted_modifier(const scripted_modifier_type *modifier)
{
	this->scripted_modifiers.erase(modifier);

	if (modifier->get_modifier() != nullptr) {
		co_await this->get_derived()->apply_modifier(modifier->get_modifier(), -1);
	}

	if (game::get()->is_running()) {
		emit this->get_derived()->scripted_modifiers_changed();
	}
}

template <typename derived_type, scope_has_scripted_modifiers scope_type>
QCoro::Task<void> scripted_modifier_scope<derived_type, scope_type>::decrement_scripted_modifiers()
{
	if (this->scripted_modifiers.empty()) {
		co_return;
	}

	std::vector<const scripted_modifier_type *> modifiers_to_remove;
	for (auto &[modifier, duration] : this->scripted_modifiers) {
		--duration;

		if (duration == 0) {
			modifiers_to_remove.push_back(modifier);
		}
	}

	for (const scripted_modifier_type *modifier : modifiers_to_remove) {
		co_await this->remove_scripted_modifier(modifier);
	}
}

template class scripted_modifier_scope<character_game_data, const character>;
template class scripted_modifier_scope<domain_game_data, const domain>;
template class scripted_modifier_scope<province_game_data, const province>;
template class scripted_modifier_scope<site_game_data, const site>;

}
