#pragma once

#include "character/character_container.h"
#include "script/scripted_modifier_container.h"
#include "spell/spell_container.h"
#include "util/fractional_int.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class character;
class civilian_unit;
class country;
class military_unit;
class portrait;
class province;
class scripted_character_modifier;
class spell;
class trait;
enum class trait_type;

template <typename scope_type>
class modifier;

class character_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::portrait* portrait READ get_portrait NOTIFY portrait_changed)
	Q_PROPERTY(const metternich::country* country READ get_country NOTIFY country_changed)
	Q_PROPERTY(int age READ get_age NOTIFY age_changed)
	Q_PROPERTY(bool dead READ is_dead NOTIFY dead_changed)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY traits_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(bool ruler READ is_ruler NOTIFY ruler_changed)
	Q_PROPERTY(QVariantList spells READ get_spells_qvariant_list NOTIFY spells_changed)
	Q_PROPERTY(bool deployable READ is_deployable NOTIFY spells_changed)

public:
	explicit character_game_data(const metternich::character *character);

	void on_setup_finished();

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	bool is_current_portrait_valid() const;
	void check_portrait();

	const metternich::country *get_country() const
	{
		return this->country;
	}

	void set_country(const metternich::country *country);

	int get_age() const;

	bool is_dead() const
	{
		return this->dead;
	}

	void set_dead(const bool dead);
	void die();

	const std::vector<const trait *> &get_traits() const
	{
		return this->traits;
	}

	QVariantList get_traits_qvariant_list() const;
	std::vector<const trait *> get_traits_of_type(const trait_type trait_type) const;
	bool can_have_trait(const trait *trait) const;
	bool has_trait(const trait *trait) const;
	void add_trait(const trait *trait);
	void remove_trait(const trait *trait);
	void sort_traits();

	const scripted_character_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_character_modifier *modifier) const;
	void add_scripted_modifier(const scripted_character_modifier *modifier, const int duration);
	void remove_scripted_modifier(const scripted_character_modifier *modifier);
	void decrement_scripted_modifiers();

	bool is_ruler() const;

	metternich::military_unit *get_military_unit() const
	{
		return this->military_unit;
	}
	
	void set_military_unit(metternich::military_unit *military_unit)
	{
		if (military_unit == this->get_military_unit()) {
			return;
		}

		this->military_unit = military_unit;
	}

	bool is_deployable() const;

	bool is_deployed() const
	{
		return this->get_military_unit() != nullptr;
	}

	void deploy_to_province(const province *province);
	void undeploy();

	metternich::civilian_unit *get_civilian_unit() const
	{
		return this->civilian_unit;
	}

	void set_civilian_unit(metternich::civilian_unit *civilian_unit)
	{
		if (civilian_unit == this->get_civilian_unit()) {
			return;
		}

		this->civilian_unit = civilian_unit;
	}

	void apply_modifier(const modifier<const metternich::character> *modifier, const int multiplier = 1);

	void remove_modifier(const modifier<const metternich::character> *modifier)
	{
		this->apply_modifier(modifier, -1);
	}

	void apply_military_unit_modifier(metternich::military_unit *military_unit, const int multiplier);

	const spell_set &get_spells() const
	{
		return this->spells;
	}

	QVariantList get_spells_qvariant_list() const;

	bool has_spell(const spell *spell) const
	{
		return this->get_spells().contains(spell);
	}

	void add_spell(const spell *spell)
	{
		this->spells.insert(spell);
		emit spells_changed();
	}

	void remove_spell(const spell *spell)
	{
		this->spells.erase(spell);
		emit spells_changed();
	}

	bool can_learn_spell(const spell *spell) const;

	bool has_learned_spell(const spell *spell) const
	{
		return this->has_spell(spell);
	}

	void learn_spell(const spell *spell);

signals:
	void portrait_changed();
	void country_changed();
	void age_changed();
	void dead_changed();
	void traits_changed();
	void scripted_modifiers_changed();
	void ruler_changed();
	void spells_changed();

private:
	const metternich::character *character = nullptr;
	const metternich::portrait *portrait = nullptr;
	const metternich::country *country = nullptr;
	bool dead = false;
	std::vector<const trait *> traits;
	scripted_character_modifier_map<int> scripted_modifiers;
	metternich::military_unit *military_unit = nullptr;
	metternich::civilian_unit *civilian_unit = nullptr;
	spell_set spells;
	spell_set item_spells;
};

}
