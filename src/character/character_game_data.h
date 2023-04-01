#pragma once

#include "character/character_container.h"
#include "script/scripted_modifier_container.h"
#include "spell/spell_container.h"
#include "util/fractional_int.h"

namespace metternich {

class character;
class country;
class icon;
class military_unit;
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

	Q_PROPERTY(metternich::icon* portrait READ get_portrait_unconst NOTIFY portrait_changed)
	Q_PROPERTY(metternich::country* employer READ get_employer_unconst NOTIFY employer_changed)
	Q_PROPERTY(int age READ get_age NOTIFY age_changed)
	Q_PROPERTY(bool dead READ is_dead NOTIFY dead_changed)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY traits_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(QVariantList spells READ get_spells_qvariant_list NOTIFY spells_changed)
	Q_PROPERTY(bool deployable READ is_deployable NOTIFY spells_changed)

public:
	explicit character_game_data(const metternich::character *character);

	void on_game_started();

	void do_turn();
	void do_events();

	const icon *get_portrait() const
	{
		return this->portrait;
	}

private:
	//for the Qt property (pointers there can't be const)
	icon *get_portrait_unconst() const
	{
		return const_cast<icon *>(this->get_portrait());
	}

public:
	bool is_current_portrait_valid() const;
	void check_portrait();

	const metternich::country *get_employer() const
	{
		return this->employer;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::country *get_employer_unconst() const
	{
		return const_cast<metternich::country *>(this->get_employer());
	}

public:
	void set_employer(const metternich::country *employer);
	void check_employer();

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
	const trait *generate_trait(const trait_type trait_type, const int max_level);
	void generate_missing_traits();
	void sort_traits();
	int get_total_expertise_trait_level() const;
	void gain_item(const trait *item);

	const scripted_character_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_character_modifier *modifier) const;
	void add_scripted_modifier(const scripted_character_modifier *modifier, const int duration);
	void remove_scripted_modifier(const scripted_character_modifier *modifier);
	void decrement_scripted_modifiers();

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

	Q_INVOKABLE QString get_country_modifier_string(const unsigned indent) const;
	Q_INVOKABLE QString get_province_modifier_string(const unsigned indent) const;

	void apply_modifier(const modifier<const metternich::character> *modifier, const int multiplier = 1);

	void remove_modifier(const modifier<const metternich::character> *modifier)
	{
		this->apply_modifier(modifier, -1);
	}

	void apply_country_modifier(const country *country, const int multiplier);
	void apply_province_modifier(const province *province, const int multiplier);
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

	bool has_item_spell(const spell *spell) const
	{
		return this->item_spells.contains(spell);
	}

	void add_item_spell(const spell *spell)
	{
		this->item_spells.insert(spell);
		this->add_spell(spell);
	}

	void remove_item_spell(const spell *spell)
	{
		this->item_spells.erase(spell);
		this->remove_spell(spell);
	}

	bool can_learn_spell(const spell *spell) const;

	bool has_learned_spell(const spell *spell) const
	{
		return this->has_spell(spell) && !this->has_item_spell(spell);
	}

	void learn_spell(const spell *spell);

signals:
	void portrait_changed();
	void employer_changed();
	void age_changed();
	void dead_changed();
	void traits_changed();
	void scripted_modifiers_changed();
	void spells_changed();

private:
	const metternich::character *character = nullptr;
	const icon *portrait = nullptr;
	const metternich::country *employer = nullptr;
	bool dead = false;
	std::vector<const trait *> traits;
	scripted_character_modifier_map<int> scripted_modifiers;
	metternich::military_unit *military_unit = nullptr;
	spell_set spells;
	spell_set item_spells;
};

}
