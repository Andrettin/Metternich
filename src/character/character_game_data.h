#pragma once

#include "character/character_container.h"
#include "database/data_entry_container.h"
#include "script/scripted_modifier_container.h"
#include "spell/spell_container.h"
#include "unit/military_unit_type_container.h"
#include "util/centesimal_int.h"

Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("domain/office.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class character;
class character_attribute;
class character_trait;
class civilian_unit;
class domain;
class enchantment;
class item;
class item_slot;
class military_unit;
class military_unit_type;
class office;
class portrait;
class province;
class scripted_character_modifier;
class species;
class spell;
enum class character_trait_type;
enum class military_unit_stat;

template <typename scope_type>
class modifier;

class character_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString titled_name READ get_titled_name_qstring NOTIFY titled_name_changed)
	Q_PROPERTY(const metternich::portrait* portrait READ get_portrait NOTIFY portrait_changed)
	Q_PROPERTY(const metternich::domain* country READ get_country NOTIFY country_changed)
	Q_PROPERTY(int age READ get_age NOTIFY age_changed)
	Q_PROPERTY(bool dead READ is_dead NOTIFY dead_changed)
	Q_PROPERTY(const metternich::character_class* character_class READ get_character_class NOTIFY character_class_changed)
	Q_PROPERTY(int level READ get_level NOTIFY level_changed)
	Q_PROPERTY(qint64 experience READ get_experience NOTIFY experience_changed)
	Q_PROPERTY(int hit_points READ get_hit_points NOTIFY hit_points_changed)
	Q_PROPERTY(int max_hit_points READ get_max_hit_points NOTIFY max_hit_points_changed)
	Q_PROPERTY(int armor_class_bonus READ get_armor_class_bonus NOTIFY armor_class_bonus_changed)
	Q_PROPERTY(int to_hit_bonus READ get_to_hit_bonus NOTIFY to_hit_bonus_changed)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY traits_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(bool ruler READ is_ruler NOTIFY ruler_changed)
	Q_PROPERTY(const metternich::office* office READ get_office NOTIFY office_changed)
	Q_PROPERTY(bool governor READ is_governor NOTIFY governor_changed)
	Q_PROPERTY(QVariantList spells READ get_spells_qvariant_list NOTIFY spells_changed)
	Q_PROPERTY(QVariantList items READ get_items_qvariant_list NOTIFY items_changed)
	Q_PROPERTY(bool deployable READ is_deployable NOTIFY spells_changed)

public:
	explicit character_game_data(const metternich::character *character);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	void apply_species_and_class(const int level);
	void apply_history(const QDate &start_date);
	void on_setup_finished();

	std::string get_titled_name() const;

	QString get_titled_name_qstring() const
	{
		return QString::fromStdString(this->get_titled_name());
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	bool is_current_portrait_valid() const;
	void check_portrait();

	const metternich::domain *get_country() const
	{
		return this->domain;
	}

	void set_country(const metternich::domain *domain);

	int get_age() const;

	bool is_dead() const
	{
		return this->dead;
	}

	void set_dead(const bool dead);
	void die();

	const metternich::character_class *get_character_class() const;
	void set_character_class(const metternich::character_class *character_class);

	int get_level() const;
	void set_level(const int level);
	void change_level(const int change);
	void on_level_gained(const int affected_level, const int multiplier);
	void check_level_experience();

	int64_t get_experience() const
	{
		return this->experience;
	}

	void set_experience(const int64_t experience)
	{
		this->change_experience(experience - this->get_experience());
	}

	void change_experience(const int64_t change);
	int64_t get_experience_for_level(const int level) const;

	const data_entry_map<character_attribute, int> &get_attribute_values() const
	{
		return this->attribute_values;
	}

	int get_attribute_value(const character_attribute *attribute) const
	{
		const auto find_iterator = this->attribute_values.find(attribute);
		if (find_iterator != this->attribute_values.end()) {
			return std::max(0, find_iterator->second);
		}

		return 0;
	}

	void change_attribute_value(const character_attribute *attribute, const int change);
	int get_primary_attribute_value() const;

	data_entry_set<character_attribute> get_main_attributes() const;

	int get_hit_dice_count() const
	{
		return this->hit_dice_count;
	}

	void change_hit_dice_count(const int change)
	{
		if (change == 0) {
			return;
		}

		this->hit_dice_count += change;
	}

	void apply_hit_dice(const dice &hit_dice);

	int get_hit_points() const
	{
		return this->hit_points;
	}

	void set_hit_points(const int hit_points);
	void change_hit_points(const int change);

	int get_max_hit_points() const
	{
		return this->max_hit_points;
	}

	void set_max_hit_points(const int hit_points);
	void change_max_hit_points(const int change);

	int get_armor_class_bonus() const
	{
		return this->armor_class_bonus;
	}

	void set_armor_class_bonus(const int bonus);
	void change_armor_class_bonus(const int change);

	const data_entry_map<species, int> &get_species_armor_class_bonuses() const
	{
		return this->species_armor_class_bonuses;
	}

	int get_species_armor_class_bonus(const species *species) const
	{
		const auto find_iterator = this->species_armor_class_bonuses.find(species);
		if (find_iterator != this->species_armor_class_bonuses.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_species_armor_class_bonus(const species *species, const int change);

	int get_to_hit_bonus() const
	{
		return this->to_hit_bonus;
	}

	void set_to_hit_bonus(const int bonus);
	void change_to_hit_bonus(const int change);

	const dice &get_damage_dice() const;

	const std::vector<const character_trait *> &get_traits() const
	{
		return this->traits;
	}

	QVariantList get_traits_qvariant_list() const;

	std::vector<const character_trait *> get_traits_of_type(const character_trait_type trait_type) const;
	Q_INVOKABLE QVariantList get_traits_of_type(const QString &trait_type_str) const;

	int get_trait_count_for_type(const character_trait_type trait_type) const
	{
		return static_cast<int>(this->get_traits_of_type(trait_type).size());
	}

	bool can_have_trait(const character_trait *trait) const;
	bool can_gain_trait(const character_trait *trait) const;
	bool has_trait(const character_trait *trait) const;
	void add_trait(const character_trait *trait);
	void remove_trait(const character_trait *trait);
	void on_trait_gained(const character_trait *trait, const int multiplier);
	[[nodiscard]] bool generate_trait(const character_trait_type trait_type, const character_attribute *target_attribute, const int target_attribute_bonus);
	[[nodiscard]] bool generate_initial_trait(const character_trait_type trait_type);
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

	const metternich::office *get_office() const
	{
		return this->office;
	}

	void set_office(const metternich::office *office);
	std::string get_office_modifier_string(const metternich::domain *domain, const metternich::office *office) const;

	Q_INVOKABLE QString get_office_modifier_qstring(const metternich::domain *domain, const metternich::office *office) const
	{
		return QString::fromStdString(this->get_office_modifier_string(domain, office));
	}

	void apply_office_modifier(const metternich::domain *domain, const metternich::office *office, const int multiplier) const;
	void apply_trait_office_modifier(const character_trait *trait, const metternich::domain *domain, const metternich::office *office, const int multiplier) const;

	bool is_governor() const;
	std::string get_governor_modifier_string(const metternich::province *province) const;

	Q_INVOKABLE QString get_governor_modifier_qstring(const metternich::province *province) const
	{
		return QString::fromStdString(this->get_governor_modifier_string(province));
	}

	void apply_governor_modifier(const metternich::province *province, const int multiplier) const;
	void apply_trait_governor_modifier(const character_trait *trait, const metternich::province *province, const int multiplier) const;

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

	void deploy_to_province(const metternich::domain *domain, const province *province);
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

	void apply_modifier(const modifier<const metternich::character> *modifier, const int multiplier);
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

	const std::vector<qunique_ptr<item>> &get_items() const
	{
		return this->items;
	}

	QVariantList get_items_qvariant_list() const;
	void add_item(qunique_ptr<item> &&item);
	void remove_item(item *item);

	const std::vector<item *> &get_equipped_items(const item_slot *slot) const
	{
		const auto find_iterator = this->equipped_items.find(slot);

		if (find_iterator != this->equipped_items.end()) {
			return find_iterator->second;
		}

		static const std::vector<item *> empty_vector;
		return empty_vector;
	}

	int get_equipped_item_count(const item_slot *slot) const
	{
		return static_cast<int>(this->get_equipped_items(slot).size());
	}

	Q_INVOKABLE bool can_equip_item(const metternich::item *item, const bool ignore_already_equipped) const;
	Q_INVOKABLE void equip_item(metternich::item *item);
	Q_INVOKABLE void deequip_item(metternich::item *item);
	void on_item_equipped(const item *item, const int multiplier);
	void on_item_equipped_with_enchantment(const enchantment *enchantment, const int multiplier);

	const centesimal_int &get_commanded_military_unit_stat_modifier(const military_unit_stat stat) const
	{
		const auto find_iterator = this->commanded_military_unit_stat_modifiers.find(stat);

		if (find_iterator != this->commanded_military_unit_stat_modifiers.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_commanded_military_unit_stat_modifier(const military_unit_stat stat, const centesimal_int &value);

	void change_commanded_military_unit_stat_modifier(const military_unit_stat stat, const centesimal_int &change)
	{
		this->set_commanded_military_unit_stat_modifier(stat, this->get_commanded_military_unit_stat_modifier(stat) + change);
	}

	const centesimal_int &get_commanded_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat) const
	{
		const auto find_iterator = this->commanded_military_unit_type_stat_modifiers.find(type);

		if (find_iterator != this->commanded_military_unit_type_stat_modifiers.end()) {
			const auto sub_find_iterator = find_iterator->second.find(stat);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_commanded_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &value);

	void change_commanded_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &change)
	{
		this->set_commanded_military_unit_type_stat_modifier(type, stat, this->get_commanded_military_unit_type_stat_modifier(type, stat) + change);
	}

signals:
	void titled_name_changed();
	void portrait_changed();
	void country_changed();
	void age_changed();
	void dead_changed();
	void character_class_changed();
	void level_changed();
	void experience_changed();
	void hit_points_changed();
	void max_hit_points_changed();
	void armor_class_bonus_changed();
	void species_armor_class_bonuses_changed();
	void to_hit_bonus_changed();
	void traits_changed();
	void scripted_modifiers_changed();
	void ruler_changed();
	void office_changed();
	void governor_changed();
	void spells_changed();
	void items_changed();
	void equipped_items_changed();

private:
	const metternich::character *character = nullptr;
	const metternich::portrait *portrait = nullptr;
	const metternich::domain *domain = nullptr;
	bool dead = false;
	const character_class *character_class = nullptr;
	int level = 0;
	int64_t experience = 0;
	data_entry_map<character_attribute, int> attribute_values;
	int hit_dice_count = 0;
	int hit_points = 0;
	int max_hit_points = 0;
	int armor_class_bonus = 0;
	data_entry_map<species, int> species_armor_class_bonuses; //armor class bonuses when attacked by certain species
	int to_hit_bonus = 0;
	std::vector<const character_trait *> traits;
	scripted_character_modifier_map<int> scripted_modifiers;
	const metternich::office *office = nullptr;
	metternich::military_unit *military_unit = nullptr;
	metternich::civilian_unit *civilian_unit = nullptr;
	spell_set spells;
	spell_set item_spells;
	std::vector<qunique_ptr<item>> items;
	data_entry_map<item_slot, std::vector<item *>> equipped_items;
	std::map<military_unit_stat, centesimal_int> commanded_military_unit_stat_modifiers;
	military_unit_type_map<std::map<military_unit_stat, centesimal_int>> commanded_military_unit_type_stat_modifiers;
};

}
