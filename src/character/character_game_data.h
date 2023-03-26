#pragma once

#include "character/character_container.h"
#include "script/opinion_modifier_container.h"
#include "script/scripted_modifier_container.h"
#include "spell/spell_container.h"
#include "util/fractional_int.h"

namespace metternich {

class character;
class country;
class icon;
class military_unit;
class opinion_modifier;
class province;
class scripted_character_modifier;
class spell;
class trait;
enum class attribute;
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
	Q_PROPERTY(int primary_attribute_value READ get_primary_attribute_value NOTIFY attributes_changed)
	Q_PROPERTY(int diplomacy READ get_diplomacy NOTIFY attributes_changed)
	Q_PROPERTY(int martial READ get_martial NOTIFY attributes_changed)
	Q_PROPERTY(int stewardship READ get_stewardship NOTIFY attributes_changed)
	Q_PROPERTY(int intrigue READ get_intrigue NOTIFY attributes_changed)
	Q_PROPERTY(int learning READ get_learning NOTIFY attributes_changed)
	Q_PROPERTY(int prowess READ get_prowess NOTIFY attributes_changed)
	Q_PROPERTY(int vitality READ get_vitality NOTIFY attributes_changed)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY traits_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(metternich::character* spouse READ get_spouse_unconst NOTIFY spouse_changed)
	Q_PROPERTY(int loyalty READ get_loyalty_int NOTIFY loyalty_changed)
	Q_PROPERTY(int wealth READ get_wealth NOTIFY wealth_changed)
	Q_PROPERTY(int prestige READ get_prestige_int NOTIFY prestige_changed)
	Q_PROPERTY(int piety READ get_piety_int NOTIFY piety_changed)
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
	void generate_expertise_traits();
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

	int get_unclamped_attribute_value(const attribute attribute) const
	{
		const auto find_iterator = this->attribute_values.find(attribute);
		if (find_iterator != this->attribute_values.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	int get_attribute_value(const attribute attribute) const;
	void set_attribute_value(const attribute attribute, const int value);

	void change_attribute_value(const attribute attribute, const int change)
	{
		this->set_attribute_value(attribute, this->get_unclamped_attribute_value(attribute) + change);
	}

	int get_primary_attribute_value() const;
	int get_diplomacy() const;
	int get_martial() const;
	int get_stewardship() const;
	int get_intrigue() const;
	int get_learning() const;
	int get_prowess() const;
	int get_vitality() const;

	const metternich::character *get_spouse() const
	{
		return this->spouse;
	}

private:
	//for the Qt property (pointers there can't be const)
	metternich::character *get_spouse_unconst() const
	{
		return const_cast<metternich::character *>(this->get_spouse());
	}

public:
	void set_spouse(const metternich::character *spouse, const bool matrilineal = false);

	bool is_married() const
	{
		return this->get_spouse() != nullptr;
	}

	bool is_married_matrilineally() const;
	bool is_subordinate_spouse() const;

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

	const centesimal_int &get_loyalty() const;

	const centesimal_int &get_unclamped_loyalty() const
	{
		return this->loyalty;
	}

	int get_loyalty_int() const
	{
		return this->get_loyalty().to_int();
	}

	void set_loyalty(const centesimal_int &loyalty)
	{
		if (loyalty == this->get_unclamped_loyalty()) {
			return;
		}

		this->loyalty = loyalty;

		emit loyalty_changed();
	}

	void change_loyalty(const centesimal_int &change)
	{
		this->set_loyalty(this->get_unclamped_loyalty() + change);
	}

	int get_wealth() const
	{
		return this->wealth;
	}

	void set_wealth(const int wealth)
	{
		if (wealth == this->get_wealth()) {
			return;
		}

		this->wealth = wealth;

		emit wealth_changed();
	}

	void change_wealth(const int change)
	{
		this->set_wealth(this->get_wealth() + change);
	}

	const centesimal_int &get_prestige() const
	{
		return this->prestige;
	}

	int get_prestige_int() const
	{
		return this->prestige.to_int();
	}

	void set_prestige(const centesimal_int &prestige)
	{
		if (prestige == this->get_prestige()) {
			return;
		}

		this->prestige = prestige;

		emit prestige_changed();
	}

	void change_prestige(const centesimal_int &change)
	{
		this->set_prestige(this->get_prestige() + change);
	}

	const centesimal_int &get_piety() const
	{
		return this->piety;
	}

	int get_piety_int() const
	{
		return this->get_piety().to_int();
	}

	void set_piety(const centesimal_int &piety)
	{
		if (piety == this->get_piety()) {
			return;
		}

		this->piety = piety;

		emit piety_changed();
	}

	void change_piety(const centesimal_int &change)
	{
		this->set_piety(this->get_piety() + change);
	}

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

	const centesimal_int &get_quarterly_prestige() const
	{
		return this->quarterly_prestige;
	}

	void change_quarterly_prestige(const centesimal_int &change);

	const centesimal_int &get_quarterly_piety() const
	{
		return this->quarterly_piety;
	}

	void change_quarterly_piety(const centesimal_int &change);

	int get_opinion_of(const metternich::character *other) const;

	int get_base_opinion(const metternich::character *other) const
	{
		const auto find_iterator = this->base_opinions.find(other);
		if (find_iterator != this->base_opinions.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_base_opinion(const metternich::character *other, const int opinion)
	{
		if (opinion == this->get_base_opinion(other)) {
			return;
		}

		if (opinion == 0) {
			this->base_opinions.erase(other);
		} else {
			this->base_opinions[other] = opinion;
		}
	}

	void change_base_opinion(const metternich::character *other, const int change)
	{
		this->set_base_opinion(other, this->get_base_opinion(other) + change);
	}

	const opinion_modifier_map<int> &get_opinion_modifiers_for(const metternich::character *other) const
	{
		static const opinion_modifier_map<int> empty_map;

		const auto find_iterator = this->opinion_modifiers.find(other);
		if (find_iterator != this->opinion_modifiers.end()) {
			return find_iterator->second;
		}

		return empty_map;
	}

	void add_opinion_modifier(const metternich::character *other, const opinion_modifier *modifier, const int duration);
	void remove_opinion_modifier(const metternich::character *other, const opinion_modifier *modifier);

signals:
	void portrait_changed();
	void employer_changed();
	void age_changed();
	void dead_changed();
	void traits_changed();
	void scripted_modifiers_changed();
	void attributes_changed();
	void spouse_changed();
	void loyalty_changed();
	void wealth_changed();
	void prestige_changed();
	void piety_changed();
	void spells_changed();

private:
	const metternich::character *character = nullptr;
	const icon *portrait = nullptr;
	const metternich::country *employer = nullptr;
	bool dead = false;
	std::vector<const trait *> traits;
	scripted_character_modifier_map<int> scripted_modifiers;
	std::map<attribute, int> attribute_values;
	const metternich::character *spouse = nullptr;
	bool matrilineal_marriage = false;
	metternich::military_unit *military_unit = nullptr;
	centesimal_int loyalty;
	int wealth = 0;
	centesimal_int prestige;
	centesimal_int piety;
	spell_set spells;
	spell_set item_spells;
	centesimal_int quarterly_prestige;
	centesimal_int quarterly_piety;
	character_map<int> base_opinions;
	character_map<opinion_modifier_map<int>> opinion_modifiers;
};

}
