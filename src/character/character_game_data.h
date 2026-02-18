#pragma once

#include "character/character_container.h"
#include "database/data_entry_container.h"
#include "domain/domain_container.h"
#include "script/scripted_modifier_container.h"
#include "spell/spell_container.h"
#include "unit/military_unit_type_container.h"
#include "util/centesimal_int.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("domain/office.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace archimedes {
	class dice;
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class bloodline;
class character;
class character_attribute;
class domain;
class enchantment;
class item;
class item_material;
class item_slot;
class item_type;
class military_unit;
class military_unit_type;
class office;
class portrait;
class province;
class saving_throw_type;
class scripted_character_modifier;
class site;
class skill;
class species;
class spell;
class status_effect;
class trait;
class trait_type;
enum class age_category;
enum class military_unit_stat;

template <typename scope_type>
class modifier;

class character_game_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString full_name READ get_full_name_qstring NOTIFY full_name_changed)
	Q_PROPERTY(QString titled_name READ get_titled_name_qstring NOTIFY titled_name_changed)
	Q_PROPERTY(const metternich::portrait* portrait READ get_portrait NOTIFY portrait_changed)
	Q_PROPERTY(const metternich::icon* icon READ get_icon NOTIFY icon_changed)
	Q_PROPERTY(const metternich::domain* domain READ get_domain NOTIFY domain_changed)
	Q_PROPERTY(int age READ get_age NOTIFY age_changed)
	Q_PROPERTY(bool dead READ is_dead NOTIFY dead_changed)
	Q_PROPERTY(QVariantList children READ get_children_qvariant_list NOTIFY age_changed)
	Q_PROPERTY(QVariantList dynastic_children READ get_dynastic_children_qvariant_list NOTIFY age_changed)
	Q_PROPERTY(const metternich::character_class* character_class READ get_character_class NOTIFY character_class_changed)
	Q_PROPERTY(int level READ get_level NOTIFY level_changed)
	Q_PROPERTY(qint64 experience READ get_experience NOTIFY experience_changed)
	Q_PROPERTY(const metternich::bloodline* bloodline READ get_bloodline NOTIFY bloodline_changed)
	Q_PROPERTY(int bloodline_strength READ get_bloodline_strength NOTIFY bloodline_strength_changed)
	Q_PROPERTY(int reputation READ get_reputation NOTIFY reputation_changed)
	Q_PROPERTY(int hit_points READ get_hit_points NOTIFY hit_points_changed)
	Q_PROPERTY(int max_hit_points READ get_max_hit_points NOTIFY max_hit_points_changed)
	Q_PROPERTY(int armor_class_bonus READ get_armor_class_bonus NOTIFY armor_class_bonus_changed)
	Q_PROPERTY(int to_hit_bonus READ get_to_hit_bonus NOTIFY to_hit_bonus_changed)
	Q_PROPERTY(int damage_bonus READ get_damage_bonus NOTIFY damage_bonus_changed)
	Q_PROPERTY(int range READ get_range NOTIFY range_changed)
	Q_PROPERTY(int movement READ get_movement NOTIFY movement_changed)
	Q_PROPERTY(int combat_movement READ get_combat_movement NOTIFY movement_changed)
	Q_PROPERTY(QVariantList saving_throw_bonuses READ get_saving_throw_bonuses_qvariant_list NOTIFY saving_throw_bonuses_changed)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY traits_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(bool ruler READ is_ruler NOTIFY ruler_changed)
	Q_PROPERTY(const metternich::office* office READ get_office NOTIFY office_changed)
	Q_PROPERTY(QVariantList spells READ get_spells_qvariant_list NOTIFY spells_changed)
	Q_PROPERTY(QVariantList items READ get_items_qvariant_list NOTIFY items_changed)
	Q_PROPERTY(bool deployable READ is_deployable NOTIFY spells_changed)
	Q_PROPERTY(QVariantList status_effects READ get_status_effects_qvariant_list NOTIFY status_effect_rounds_changed)

public:
	explicit character_game_data(const metternich::character *character);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	void apply_species_and_class(const int level);
	void generate_attributes();
	void apply_bloodline();
	void apply_bloodline_from_parents();
	void add_starting_items();
	void add_starting_items(const std::vector<const item_type *> &starting_items, data_entry_set<item_slot> &filled_item_slots);
	void apply_history(const QDate &start_date);
	void on_setup_finished();

	std::string get_full_name() const;

	QString get_full_name_qstring() const
	{
		return QString::fromStdString(this->get_full_name());
	}

	Q_INVOKABLE QString get_full_name_for_domain(const metternich::domain *domain) const;

	std::string get_titled_name() const;

	QString get_titled_name_qstring() const
	{
		return QString::fromStdString(this->get_titled_name());
	}

	std::optional<std::pair<const domain *, int>> get_regnal_domain_and_number() const;
	std::optional<int> get_regnal_number_for_domain(const domain *domain) const;
	std::optional<std::pair<const domain *, int>> get_best_regnal_domain_and_number() const;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	bool is_current_portrait_valid() const;
	void check_portrait();

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_current_icon_valid() const;
	void check_icon();

	const metternich::domain *get_domain() const
	{
		return this->domain;
	}

	void set_domain(const metternich::domain *domain);

	int get_age() const;
	age_category get_age_category() const;

	bool is_dead() const
	{
		return this->dead;
	}

	void set_dead(const bool dead);
	void die();

	bool exists() const;
	bool has_ever_existed() const;
	std::vector<const metternich::character *> get_children() const;
	QVariantList get_children_qvariant_list() const;
	std::vector<const metternich::character *> get_dynastic_children() const;
	QVariantList get_dynastic_children_qvariant_list() const;

	const QDate &get_birth_date() const
	{
		return this->birth_date;
	}

	void set_birth_date(const QDate &date)
	{
		this->birth_date = date;
	}

	const QDate &get_death_date() const
	{
		return this->death_date;
	}

	void set_death_date(const QDate &date)
	{
		this->death_date = date;
	}

	const QDate &get_start_date() const
	{
		return this->start_date;
	}

	void set_start_date(const QDate &date)
	{
		this->start_date = date;
	}

	const site *get_home_site() const
	{
		return this->home_site;
	}

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
	int64_t get_experience_award() const;
	void change_experience_award(const int64_t change);

	bool is_deity() const;

	const metternich::bloodline *get_bloodline() const
	{
		return this->bloodline;
	}

	void set_bloodline(const metternich::bloodline *bloodline);

	int get_bloodline_strength() const
	{
		return this->bloodline_strength;
	}

	void set_bloodline_strength(const int bloodline_strength);

	int get_reputation() const
	{
		return this->reputation;
	}

	void set_reputation(const int reputation);

	void change_reputation(const int change)
	{
		this->set_reputation(this->get_reputation() + change);
	}

	const data_entry_map<character_attribute, int> &get_attribute_values() const
	{
		return this->attribute_values;
	}

	int get_attribute_value(const character_attribute *attribute) const
	{
		const auto find_iterator = this->attribute_values.find(attribute);
		if (find_iterator != this->attribute_values.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_attribute_value(const character_attribute *attribute, const int change);
	int get_primary_attribute_value() const;
	data_entry_set<character_attribute> get_main_attributes() const;
	bool do_attribute_check(const character_attribute *attribute, const int roll_modifier) const;
	int get_attribute_check_chance(const character_attribute *attribute, const int roll_modifier) const;

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
	void remove_hit_dice(const dice &hit_dice);

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

	int get_hit_point_bonus_per_hit_dice() const
	{
		return this->hit_point_bonus_per_hit_dice;
	}

	void set_hit_point_bonus_per_hit_dice(const int bonus);
	void change_hit_point_bonus_per_hit_dice(const int change);

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

	int get_damage_bonus() const
	{
		return this->damage_bonus;
	}

	void set_damage_bonus(const int bonus);
	void change_damage_bonus(const int change);

	int get_range() const
	{
		return this->range;
	}

	void set_range(const int range);
	void change_range(const int change);

	int get_movement() const
	{
		return this->movement;
	}

	void set_movement(const int movement);
	void change_movement(const int change);
	int get_combat_movement() const;

	const data_entry_map<saving_throw_type, int> &get_saving_throw_bonuses() const
	{
		return this->saving_throw_bonuses;
	}

	QVariantList get_saving_throw_bonuses_qvariant_list() const;

	int get_saving_throw_bonus(const saving_throw_type *type) const
	{
		const auto find_iterator = this->saving_throw_bonuses.find(type);
		if (find_iterator != this->saving_throw_bonuses.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_saving_throw_bonus(const saving_throw_type *type, const int change);
	bool do_saving_throw(const saving_throw_type *saving_throw_type, const int roll_modifier = 0) const;

	bool is_skill_trained(const skill *skill) const;
	void change_skill_training(const skill *skill, const int change);

	const data_entry_map<skill, int> &get_skill_values() const
	{
		return this->skill_values;
	}

	int get_skill_value(const skill *skill) const
	{
		const auto find_iterator = this->skill_values.find(skill);
		if (find_iterator != this->skill_values.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_skill_value(const skill *skill, const int change);
	bool do_skill_check(const skill *skill, const int roll_modifier, const site *location) const;
	int get_skill_check_chance(const skill *skill, const int roll_modifier, const site *location) const;

	const data_entry_map<trait, int> &get_trait_counts() const
	{
		return this->trait_counts;
	}

	int get_trait_count(const trait *trait) const
	{
		const auto find_iterator = this->trait_counts.find(trait);

		if (find_iterator != this->trait_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void change_trait_count(const trait *trait, const int change);

	QVariantList get_traits_qvariant_list() const;

	std::vector<const trait *> get_traits_of_type(const trait_type *trait_type) const;
	Q_INVOKABLE QVariantList get_traits_of_type(const QString &trait_type_str) const;

	int get_trait_count_for_type(const trait_type *trait_type) const
	{
		return static_cast<int>(this->get_traits_of_type(trait_type).size());
	}

	bool can_have_trait(const trait *trait) const;
	bool can_gain_trait(const trait *trait) const;
	bool has_trait(const trait *trait) const;
	void on_trait_gained(const trait *trait, const int multiplier);
	void add_trait_of_type(const trait_type *trait_type);
	void remove_trait_of_type(const trait_type *trait_type);
	[[nodiscard]] bool generate_trait(const trait_type *trait_type, const character_attribute *target_attribute, const int target_attribute_bonus);
	[[nodiscard]] bool generate_initial_trait(const trait_type *trait_type);
	Q_INVOKABLE void on_trait_chosen(const metternich::trait *trait, const metternich::trait_type *trait_type);
	std::vector<const trait *> get_potential_traits_from_list(const std::vector<const trait *> &traits) const;

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
	void apply_trait_office_modifier(const trait *trait, const metternich::domain *domain, const metternich::office *office, const int multiplier) const;

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
	bool has_item(const item_type *item_type) const;
	void add_item(qunique_ptr<item> &&item);
	void remove_item(item *item);
	void remove_item(const item_type *item_type, const item_material *material, const enchantment *enchantment);

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

	void set_target_traits(const std::vector<const trait *> &traits)
	{
		this->target_traits = traits;
	}

	bool has_any_status_effect() const
	{
		return !this->status_effect_rounds.empty();
	}

	bool has_status_effect(const status_effect *status_effect) const
	{
		return this->get_status_effect_rounds(status_effect) > 0;
	}

	QVariantList get_status_effects_qvariant_list() const;

	int get_status_effect_rounds(const status_effect *status_effect) const
	{
		const auto find_iterator = this->status_effect_rounds.find(status_effect);

		if (find_iterator != this->status_effect_rounds.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_status_effect_rounds(const status_effect *status_effect, const int rounds);

	void change_status_effect_rounds(const status_effect *status_effect, const int change)
	{
		this->set_status_effect_rounds(status_effect, this->get_status_effect_rounds(status_effect) + change);
	}

	void decrement_status_effect_rounds();

	const domain_set &get_ruled_domains() const
	{
		return this->ruled_domains;
	}

	void add_ruled_domain(const metternich::domain *domain)
	{
		this->ruled_domains.insert(domain);
	}
	
	const domain_set &get_reigned_domains() const
	{
		return this->reigned_domains;
	}

	void add_reigned_domain(const metternich::domain *domain)
	{
		this->reigned_domains.insert(domain);
	}
	
	const site *get_location() const;

signals:
	void full_name_changed();
	void titled_name_changed();
	void portrait_changed();
	void icon_changed();
	void domain_changed();
	void age_changed();
	void dead_changed();
	void character_class_changed();
	void level_changed();
	void experience_changed();
	void bloodline_changed();
	void bloodline_strength_changed();
	void reputation_changed();
	void hit_points_changed();
	void max_hit_points_changed();
	void armor_class_bonus_changed();
	void species_armor_class_bonuses_changed();
	void to_hit_bonus_changed();
	void damage_bonus_changed();
	void range_changed();
	void movement_changed();
	void saving_throw_bonuses_changed();
	void skill_trainings_changed();
	void skill_values_changed();
	void traits_changed();
	void scripted_modifiers_changed();
	void ruler_changed();
	void office_changed();
	void spells_changed();
	void items_changed();
	void equipped_items_changed();
	void status_effect_rounds_changed();

private:
	const metternich::character *character = nullptr;
	const metternich::portrait *portrait = nullptr;
	const metternich::icon *icon = nullptr;
	const metternich::domain *domain = nullptr;
	bool dead = false;
	QDate birth_date;
	QDate death_date;
	QDate start_date;
	const site *home_site = nullptr;
	const metternich::character_class *character_class = nullptr;
	int level = 0;
	int64_t experience = 0;
	int64_t experience_award = 0; //the experience award for defeating the character
	const metternich::bloodline *bloodline = nullptr;
	int bloodline_strength = 0;
	int reputation = 0;
	data_entry_map<character_attribute, int> attribute_values;
	int hit_dice_count = 0;
	int hit_points = 0;
	int max_hit_points = 0;
	std::map<dice, std::vector<int>> hit_dice_roll_results;
	int hit_point_bonus_per_hit_dice = 0;
	int armor_class_bonus = 0;
	data_entry_map<species, int> species_armor_class_bonuses; //armor class bonuses when attacked by certain species
	int to_hit_bonus = 0;
	int damage_bonus = 0;
	int range = 1;
	int movement = 0;
	data_entry_map<saving_throw_type, int> saving_throw_bonuses;
	data_entry_map<skill, int> skill_trainings;
	data_entry_map<skill, int> skill_values;
	data_entry_map<trait, int> trait_counts;
	data_entry_map<trait_type, std::vector<const trait *>> trait_choices;
	scripted_character_modifier_map<int> scripted_modifiers;
	const metternich::office *office = nullptr;
	metternich::military_unit *military_unit = nullptr;
	spell_set spells;
	spell_set item_spells;
	std::vector<qunique_ptr<item>> items;
	data_entry_map<item_slot, std::vector<item *>> equipped_items;
	std::map<military_unit_stat, centesimal_int> commanded_military_unit_stat_modifiers;
	military_unit_type_map<std::map<military_unit_stat, centesimal_int>> commanded_military_unit_type_stat_modifiers;
	std::vector<const trait *> target_traits;
	data_entry_map<status_effect, int> status_effect_rounds; //doesn't need to be saved since the game cannot be saved from within combat
	domain_set ruled_domains; //domains that this character has ever ruled
	domain_set reigned_domains; //domains that this character has ever ruled with a regnal number
};

}
