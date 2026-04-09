#pragma once

#include "character/character_container.h"
#include "database/data_entry_container.h"
#include "domain/domain_container.h"
#include "script/scripted_modifier_container.h"
#include "unit/military_unit_type_container.h"
#include "util/centesimal_int.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("domain/domain.h")
Q_MOC_INCLUDE("domain/office.h")
Q_MOC_INCLUDE("unit/civilian_unit.h")
Q_MOC_INCLUDE("unit/military_unit.h")
Q_MOC_INCLUDE("ui/icon.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace archimedes {
	class dice;
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class bloodline;
class building_item_slot;
class character;
class character_attribute;
class character_stat;
class civilian_unit;
class domain;
class domain_skill;
class enchantment;
class flag;
class item;
class item_material;
class item_slot;
class item_type;
class military_unit;
class military_unit_type;
class office;
class portrait;
class province;
class recipe;
class saving_throw_type;
class scripted_character_modifier;
class site;
class skill;
class sound;
class species;
class spell;
class status_effect;
class trait;
class trait_type;
enum class age_category;
enum class character_modifier_type;
enum class military_unit_stat;
struct context;
struct item_key;
struct recipe_material;

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
	Q_PROPERTY(int caster_level READ get_caster_level NOTIFY caster_level_changed)
	Q_PROPERTY(const metternich::bloodline* bloodline READ get_bloodline NOTIFY bloodline_changed)
	Q_PROPERTY(int bloodline_strength READ get_bloodline_strength NOTIFY bloodline_strength_changed)
	Q_PROPERTY(int reputation READ get_reputation NOTIFY reputation_changed)
	Q_PROPERTY(int health READ get_health NOTIFY health_changed)
	Q_PROPERTY(int max_health READ get_max_health NOTIFY max_health_changed)
	Q_PROPERTY(int mana READ get_mana NOTIFY mana_changed)
	Q_PROPERTY(int max_mana READ get_max_mana NOTIFY max_mana_changed)
	Q_PROPERTY(int craft READ get_craft NOTIFY craft_changed)
	Q_PROPERTY(int max_craft READ get_max_craft NOTIFY max_craft_changed)
	Q_PROPERTY(int armor_class_bonus READ get_armor_class_bonus NOTIFY armor_class_bonus_changed)
	Q_PROPERTY(int to_hit_bonus READ get_to_hit_bonus NOTIFY to_hit_bonus_changed)
	Q_PROPERTY(int damage_bonus READ get_damage_bonus NOTIFY damage_bonus_changed)
	Q_PROPERTY(int range READ get_range NOTIFY range_changed)
	Q_PROPERTY(int movement READ get_movement NOTIFY movement_changed)
	Q_PROPERTY(int combat_movement READ get_combat_movement NOTIFY movement_changed)
	Q_PROPERTY(int initiative_bonus READ get_initiative_bonus NOTIFY initiative_bonus_changed)
	Q_PROPERTY(QVariantList saving_throw_bonuses READ get_saving_throw_bonuses_qvariant_list NOTIFY saving_throw_bonuses_changed)
	Q_PROPERTY(QVariantList traits READ get_traits_qvariant_list NOTIFY traits_changed)
	Q_PROPERTY(QVariantList scripted_modifiers READ get_scripted_modifiers_qvariant_list NOTIFY scripted_modifiers_changed)
	Q_PROPERTY(bool ruler READ is_ruler NOTIFY ruler_changed)
	Q_PROPERTY(const metternich::office* office READ get_office NOTIFY office_changed)
	Q_PROPERTY(QVariantList spells READ get_spells_qvariant_list NOTIFY spells_changed)
	Q_PROPERTY(QVariantList combat_spells READ get_combat_spells_qvariant_list NOTIFY spells_changed)
	Q_PROPERTY(QVariantList battle_spells READ get_battle_spells_qvariant_list NOTIFY spells_changed)
	Q_PROPERTY(QVariantList recipes READ get_recipes_qvariant_list NOTIFY recipes_changed)
	Q_PROPERTY(QVariantList items READ get_items_qvariant_list NOTIFY items_changed)
	Q_PROPERTY(QVariantList unequipped_items READ get_unequipped_items_qvariant_list NOTIFY unequipped_items_changed)
	Q_PROPERTY(bool deployable READ is_deployable CONSTANT)
	Q_PROPERTY(const metternich::military_unit* military_unit READ get_military_unit NOTIFY military_unit_changed)
	Q_PROPERTY(const metternich::civilian_unit* civilian_unit READ get_civilian_unit NOTIFY civilian_unit_changed)
	Q_PROPERTY(QVariantList status_effects READ get_status_effects_qvariant_list NOTIFY status_effect_durations_changed)

public:
	explicit character_game_data(const metternich::character *character);

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	[[nodiscard]] QCoro::Task<void> initialize();

	void ply_trade();
	[[nodiscard]] QCoro::Task<void> do_crafting();
	[[nodiscard]] QCoro::Task<void> do_events();

	bool is_ai() const;

	[[nodiscard]] QCoro::Task<void> apply_species_and_class(const int level, const bool apply_history);
	[[nodiscard]] QCoro::Task<void> generate_attributes();
	void apply_bloodline(const bool apply_history);
	void apply_bloodline_from_parents();
	void apply_bloodline_inheritance_investiture();
	[[nodiscard]] QCoro::Task<void> add_starting_items();
	[[nodiscard]] QCoro::Task<void> add_starting_items(const std::vector<const item_type *> &starting_items, data_entry_set<item_slot> &filled_item_slots);
	void add_starting_spells();
	void add_starting_spells(const std::vector<const spell *> &starting_spells);
	[[nodiscard]] QCoro::Task<void> apply_history(const QDate &start_date);
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
	[[nodiscard]] QCoro::Task<void> die();

	bool exists() const;
	bool has_ever_existed() const;

	bool is_playable() const;
	QString get_unplayable_reason() const;

	std::vector<const metternich::character *> get_children() const;
	QVariantList get_children_qvariant_list() const;
	std::vector<const metternich::character *> get_dynastic_children() const;
	QVariantList get_dynastic_children_qvariant_list() const;
	std::vector<const metternich::character *> get_next_of_kin(character_set &checked_characters, const bool include_parents) const;

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
	[[nodiscard]] QCoro::Task<void> set_level(const int level);
	[[nodiscard]] QCoro::Task<void> change_level(const int change);
	[[nodiscard]] QCoro::Task<void> on_level_gained(const int affected_level, const int multiplier);
	[[nodiscard]] QCoro::Task<void> check_level_experience();

	int64_t get_experience() const
	{
		return this->experience;
	}

	[[nodiscard]] QCoro::Task<void> set_experience(const int64_t experience)
	{
		co_await this->change_experience(experience - this->get_experience());
	}

	[[nodiscard]] QCoro::Task<void> change_experience(const int64_t change);
	int64_t get_experience_for_level(const int level) const;

	int get_challenge_rating() const
	{
		return this->challenge_rating;
	}

	void change_challenge_rating(const int change);
	int64_t get_experience_award() const;

	int get_caster_level() const
	{
		return this->caster_level;
	}

	void change_caster_level(const int change);

	[[nodiscard]] QCoro::Task<void> on_mythic_tier_gained(const int affected_tier, const int multiplier);
	bool is_deity() const;
	[[nodiscard]] QCoro::Task<void> on_divine_rank_gained(const int affected_rank, const int multiplier);

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
	void inherit_bloodline_from(const metternich::character *other_character);

	int get_reputation() const
	{
		return this->reputation;
	}

	void set_reputation(const int reputation);

	void change_reputation(const int change)
	{
		this->set_reputation(this->get_reputation() + change);
	}

	int get_stat_value(const character_stat *stat) const
	{
		const auto find_iterator = this->stat_values.find(stat);
		if (find_iterator != this->stat_values.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	const data_entry_map<character_stat, int> &get_stat_values() const
	{
		return this->stat_values;
	}

	[[nodiscard]] QCoro::Task<void> change_stat_value(const character_stat *stat, const int change, const bool stat_enabled, const bool affects_office_modifier);

	int get_attribute_value(const character_attribute *attribute) const;
	[[nodiscard]] QCoro::Task<void> change_attribute_value(const character_attribute *attribute, const int change);
	int get_primary_attribute_value() const;
	int get_attribute_modifier(const character_attribute *attribute) const;
	data_entry_set<character_attribute> get_main_attributes() const;
	bool do_attribute_check(const character_attribute *attribute, const int roll_modifier) const;
	int get_attribute_check_chance(const character_attribute *attribute, const int roll_modifier) const;
	[[nodiscard]] QCoro::Task<void> on_stat_value_changed(const character_stat *stat, const int new_value, const int old_value);

	int get_best_attribute_modifier(const character_attribute *attribute, const character_modifier_type modifier_type) const
	{
		const auto attribute_find_iterator = this->attribute_modifiers.find(attribute);
		if (attribute_find_iterator != this->attribute_modifiers.end()) {
			const auto modifier_type_find_iterator = attribute_find_iterator->second.find(modifier_type);
			if (modifier_type_find_iterator != attribute_find_iterator->second.end()) {
				int best_modifier = 0;

				for (const int modifier : modifier_type_find_iterator->second) {
					if (modifier > best_modifier) {
						best_modifier = modifier;
					}
				}

				return best_modifier;
			}
		}

		return 0;
	}

	[[nodiscard]] QCoro::Task<void> add_attribute_modifier(const character_attribute *attribute, const character_modifier_type modifier_type, const int modifier)
	{
		const int old_best_modifier = this->get_best_attribute_modifier(attribute, modifier_type);

		this->attribute_modifiers[attribute][modifier_type].push_back(modifier);

		const int new_best_modifier = this->get_best_attribute_modifier(attribute, modifier_type);

		if (new_best_modifier != old_best_modifier) {
			co_await this->change_attribute_value(attribute, new_best_modifier - old_best_modifier);
		}
	}

	[[nodiscard]] QCoro::Task<void> remove_attribute_modifier(const character_attribute *attribute, const character_modifier_type modifier_type, const int modifier)
	{
		auto attribute_find_iterator = this->attribute_modifiers.find(attribute);
		if (attribute_find_iterator != this->attribute_modifiers.end()) {
			auto modifier_type_find_iterator = attribute_find_iterator->second.find(modifier_type);
			if (modifier_type_find_iterator != attribute_find_iterator->second.end()) {
				const int old_best_modifier = this->get_best_attribute_modifier(attribute, modifier_type);

				std::erase(modifier_type_find_iterator->second, modifier);

				const int new_best_modifier = this->get_best_attribute_modifier(attribute, modifier_type);

				if (new_best_modifier != old_best_modifier) {
					co_await this->change_attribute_value(attribute, new_best_modifier - old_best_modifier);
				}
			}
		}
	}

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

		this->change_challenge_rating(change);
	}

	[[nodiscard]] QCoro::Task<void> apply_hit_dice(const dice &hit_dice);
	[[nodiscard]] QCoro::Task<void> remove_hit_dice(const dice &hit_dice);

	int get_health() const
	{
		return this->health;
	}

	[[nodiscard]] QCoro::Task<void> set_health(const int health);
	[[nodiscard]] QCoro::Task<void> change_health(const int change);

	int get_max_health() const
	{
		return this->max_health;
	}

	[[nodiscard]] QCoro::Task<void> set_max_health(const int health, const bool increase_health);
	[[nodiscard]] QCoro::Task<void> change_max_health(const int change, const bool increase_health);

	int get_health_bonus_per_hit_dice() const
	{
		return this->health_bonus_per_hit_dice;
	}

	[[nodiscard]] QCoro::Task<void> set_health_bonus_per_hit_dice(const int bonus);
	[[nodiscard]] QCoro::Task<void> change_health_bonus_per_hit_dice(const int change);

	int get_mana() const
	{
		return this->mana;
	}

	void set_mana(const int mana);
	void change_mana(const int change);

	int get_max_mana() const
	{
		return this->max_mana;
	}

	void set_max_mana(const int mana, const bool increase_mana);
	void change_max_mana(const int change, const bool increase_mana);

	int get_craft() const
	{
		return this->craft;
	}

	void set_craft(const int craft);
	void change_craft(const int change);

	int get_max_craft() const
	{
		return this->max_craft;
	}

	void set_max_craft(const int craft, const bool increase_craft);
	void change_max_craft(const int change, const bool increase_craft);

	[[nodiscard]] QCoro::Task<void> fully_recover()
	{
		co_await this->set_health(this->get_max_health());
		this->set_mana(this->get_max_mana());
	}

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

	int get_initiative_bonus() const
	{
		return this->initiative_bonus;
	}

	void set_initiative_bonus(const int initiative_bonus);
	void change_initiative_bonus(const int change);

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
	int get_saving_throw_chance(const saving_throw_type *saving_throw_type, const int roll_modifier = 0) const;

	bool is_skill_available(const skill *skill) const;
	bool is_skill_trained(const skill *skill) const;
	[[nodiscard]] QCoro::Task<void> change_skill_training(const skill *skill, const int change);

	int get_skill_value(const skill *skill) const;
	[[nodiscard]] QCoro::Task<void> change_skill_value(const skill *skill, const int change);
	int get_effective_skill_value(const skill *skill) const;
	bool do_skill_check(const skill *skill, const int roll_modifier, const site *location) const;
	int get_skill_check_chance(const skill *skill, const int roll_modifier, const site *location) const;

	bool has_domain_skill() const;
	int get_domain_skill_value(const domain_skill *domain_skill) const;
	[[nodiscard]] QCoro::Task<void> change_domain_skill_value(const domain_skill *domain_skill, const int change);

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

	[[nodiscard]] QCoro::Task<void> change_trait_count(const trait *trait, const int change);

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
	[[nodiscard]] QCoro::Task<void> on_trait_gained(const trait *trait, const int multiplier);
	[[nodiscard]] QCoro::Task<void> add_trait_of_type(const trait_type *trait_type);
	[[nodiscard]] QCoro::Task<void> remove_trait_of_type(const trait_type *trait_type);
	[[nodiscard]] QCoro::Task<bool> generate_trait(const trait_type *trait_type, const character_attribute *target_attribute, const int target_attribute_bonus);
	[[nodiscard]] QCoro::Task<void> on_trait_chosen_coro(const trait *trait, const trait_type *trait_type);

	Q_INVOKABLE QCoro::QmlTask on_trait_chosen(const metternich::trait *trait, const metternich::trait_type *trait_type)
	{
		return this->on_trait_chosen_coro(trait, trait_type);
	}

	std::vector<const trait *> get_potential_traits_from_list(const std::vector<const trait *> &traits) const;

	const scripted_character_modifier_map<int> &get_scripted_modifiers() const
	{
		return this->scripted_modifiers;
	}

	QVariantList get_scripted_modifiers_qvariant_list() const;
	bool has_scripted_modifier(const scripted_character_modifier *modifier) const;
	[[nodiscard]] QCoro::Task<void> add_scripted_modifier(const scripted_character_modifier *modifier, const int duration);
	[[nodiscard]] QCoro::Task<void> remove_scripted_modifier(const scripted_character_modifier *modifier);
	[[nodiscard]] QCoro::Task<void> decrement_scripted_modifiers();

	bool is_ruler() const;

	const metternich::office *get_office() const
	{
		return this->office;
	}

	void set_office(const metternich::office *office);
	int get_office_domain_attribute_modifier(const metternich::office *office, const character_attribute **output_character_attribute, const skill **output_skill) const;
	std::string get_office_modifier_string(const metternich::domain *domain, const metternich::office *office) const;

	Q_INVOKABLE QString get_office_modifier_qstring(const metternich::domain *domain, const metternich::office *office) const
	{
		return QString::fromStdString(this->get_office_modifier_string(domain, office));
	}

	[[nodiscard]] QCoro::Task<void> apply_office_modifier(const metternich::domain *domain, const metternich::office *office, const int multiplier) const;
	[[nodiscard]] QCoro::Task<void> apply_trait_office_modifier(const trait *trait, const metternich::domain *domain, const metternich::office *office, const int multiplier) const;

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

		emit military_unit_changed();
	}

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

		emit civilian_unit_changed();
	}

	bool is_deployable() const;

	bool is_deployed() const
	{
		return this->get_military_unit() != nullptr || this->get_civilian_unit() != nullptr;
	}

	[[nodiscard]] QCoro::Task<void> deploy_to_province(const metternich::domain *domain, const province *province);
	[[nodiscard]] QCoro::Task<void> undeploy();

	[[nodiscard]] QCoro::Task<void> apply_modifier(const modifier<const metternich::character> *modifier, const int multiplier);
	[[nodiscard]] QCoro::Task<void> apply_military_unit_modifier(metternich::military_unit *military_unit, const int multiplier);

	const std::vector<const spell *> &get_spells() const
	{
		return this->spells;
	}

	QVariantList get_spells_qvariant_list() const;
	bool has_spell(const spell *spell) const;

	void add_spell(const spell *spell)
	{
		this->spells.push_back(spell);
		this->sort_spells();
		emit spells_changed();
	}

	void remove_spell(const spell *spell)
	{
		std::erase(this->spells, spell);
		emit spells_changed();
	}

	bool can_learn_spell(const metternich::spell *spell, std::string *reason = nullptr) const;

	bool has_learned_spell(const spell *spell) const
	{
		return this->has_spell(spell);
	}

	void learn_spell(const metternich::spell *spell);
	Q_INVOKABLE bool can_cast_spell(const metternich::spell *spell) const;
	void sort_spells();

	QVariantList get_combat_spells_qvariant_list() const;
	QVariantList get_battle_spells_qvariant_list() const;

	const std::vector<const recipe *> &get_recipes() const
	{
		return this->recipes;
	}

	QVariantList get_recipes_qvariant_list() const;
	bool has_recipe(const recipe *recipe) const;

	void add_recipe(const recipe *recipe)
	{
		this->recipes.push_back(recipe);
		this->sort_recipes();
		emit recipes_changed();
	}

	void remove_recipe(const recipe *recipe)
	{
		std::erase(this->recipes, recipe);
		emit recipes_changed();
	}

	bool can_learn_recipe(const metternich::recipe *recipe, std::string *reason = nullptr) const;
	void learn_recipe(const metternich::recipe *recipe);
	bool has_recipe_material(const recipe_material &material) const;
	Q_INVOKABLE bool can_craft_recipe(const metternich::recipe *recipe) const;
	[[nodiscard]] QCoro::Task<void> craft_recipe_coro(const recipe *recipe);

	Q_INVOKABLE QCoro::QmlTask craft_recipe(const metternich::recipe *recipe)
	{
		return this->craft_recipe_coro(recipe);
	}

	void sort_recipes();

	int64_t get_wealth() const;
	void change_wealth(const int64_t change);

	const std::vector<qunique_ptr<item>> &get_items() const
	{
		return this->items;
	}

	QVariantList get_items_qvariant_list() const;
	QVariantList get_unequipped_items_qvariant_list() const;
	bool has_item(const item_type *item_type) const;
	[[nodiscard]] QCoro::Task<void> add_item(qunique_ptr<item> &&item);
	[[nodiscard]] QCoro::Task<qunique_ptr<item>> take_item(metternich::item *item);
	[[nodiscard]] QCoro::Task<void> remove_item_coro(item *item);

	Q_INVOKABLE QCoro::QmlTask remove_item(metternich::item *item)
	{
		return this->remove_item_coro(item);
	}

	[[nodiscard]] QCoro::Task<void> remove_item(const item_type *item_type, const item_material *material, const enchantment *enchantment, const spell *spell, const recipe *recipe);

	bool can_consume_item(const item_key &item_key, std::string *reason) const;
	Q_INVOKABLE bool can_consume_item(const metternich::item *item) const;

	[[nodiscard]] QCoro::Task<void> consume_item_coro(item *item);

	Q_INVOKABLE QCoro::QmlTask consume_item(metternich::item *item)
	{
		return this->consume_item_coro(item);
	}

	[[nodiscard]] QCoro::Task<void> on_item_consumed(const item *item);
	[[nodiscard]] QCoro::Task<void> on_item_consumed_with_enchantment(const enchantment *enchantment);

	const std::vector<item *> &get_equipped_items(const item_slot *slot) const
	{
		const auto find_iterator = this->equipped_items.find(slot);

		if (find_iterator != this->equipped_items.end()) {
			return find_iterator->second;
		}

		static const std::vector<item *> empty_vector;
		return empty_vector;
	}

	Q_INVOKABLE const metternich::item *get_equipped_item(const metternich::item_slot *slot, const int slot_index) const;

	int get_equipped_item_count(const item_slot *slot) const
	{
		return static_cast<int>(this->get_equipped_items(slot).size());
	}

	bool can_equip_item(const item_key &item_key, const bool ignore_already_equipped, const std::optional<int64_t> &already_equipped_ignore_price_threshold) const;
	Q_INVOKABLE bool can_equip_item(const metternich::item *item, const bool ignore_already_equipped) const;
	[[nodiscard]] QCoro::Task<void> equip_item_coro(item *item);

	Q_INVOKABLE QCoro::QmlTask equip_item(metternich::item *item)
	{
		return this->equip_item_coro(item);
	}

	[[nodiscard]] QCoro::Task<void> deequip_item_coro(item *item);

	Q_INVOKABLE QCoro::QmlTask deequip_item(metternich::item *item)
	{
		return this->deequip_item_coro(item);
	}

	[[nodiscard]] QCoro::Task<void> on_item_equipped(const item *item, const int multiplier);
	[[nodiscard]] QCoro::Task<void> on_item_equipped_with_enchantment(const enchantment *enchantment, const int multiplier);

	bool can_use_item(const metternich::item *item, std::string *reason) const;

	Q_INVOKABLE bool can_use_item(const metternich::item *item) const
	{
		return this->can_use_item(item, nullptr);
	}

	[[nodiscard]] QCoro::Task<void> use_item_coro(item *item);

	Q_INVOKABLE QCoro::QmlTask use_item(metternich::item *item)
	{
		return this->use_item_coro(item);
	}

	bool can_use_enchantment(const enchantment *enchantment) const;

	Q_INVOKABLE bool can_sell_item(const metternich::item *item) const;
	[[nodiscard]] QCoro::Task<void> sell_item_coro(item *item);

	Q_INVOKABLE QCoro::QmlTask sell_item(metternich::item *item)
	{
		return this->sell_item_coro(item);
	}

	std::vector<building_item_slot *> get_accessible_building_item_slots() const;

	bool can_craft_items() const;

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
		return !this->status_effect_durations.empty();
	}

	bool has_any_status_effect_with_less_than_duration(const std::chrono::seconds &duration_threshold) const
	{
		for (const auto &[status_effect, duration] : this->status_effect_durations) {
			if (duration < duration_threshold) {
				return true;
			}
		}

		return false;
	}

	bool has_status_effect(const status_effect *status_effect) const
	{
		return this->status_effect_durations.find(status_effect) != this->status_effect_durations.end();
	}

	QVariantList get_status_effects_qvariant_list() const;

	const data_entry_map<status_effect, std::chrono::seconds> &get_status_effect_durations() const
	{
		return this->status_effect_durations;
	}

	const std::chrono::seconds &get_status_effect_duration(const status_effect *status_effect) const
	{
		const auto find_iterator = this->status_effect_durations.find(status_effect);

		if (find_iterator != this->status_effect_durations.end()) {
			return find_iterator->second;
		}

		static constexpr std::chrono::seconds zero = std::chrono::seconds(0);
		return zero;
	}

	[[nodiscard]] QCoro::Task<void> set_status_effect_duration(const status_effect *status_effect, const std::chrono::seconds &duration);

	[[nodiscard]] QCoro::Task<void> change_status_effect_duration(const status_effect *status_effect, const std::chrono::seconds &change)
	{
		co_await this->set_status_effect_duration(status_effect, this->get_status_effect_duration(status_effect) + change);
	}

	[[nodiscard]]
	QCoro::Task<void> decrement_status_effect_durations(const std::chrono::seconds &decrement, context &ctx);

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

	bool has_flag(const flag *flag) const
	{
		return this->flags.contains(flag);
	}

	void set_flag(const flag *flag)
	{
		this->flags.insert(flag);
	}

	void clear_flag(const flag *flag)
	{
		this->flags.erase(flag);
	}

	[[nodiscard]] QCoro::Task<void> ai_buy_items();
	[[nodiscard]] QCoro::Task<void> ai_sell_items();

	const sound *get_attack_sound() const;

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
	void caster_level_changed();
	void bloodline_changed();
	void bloodline_strength_changed();
	void reputation_changed();
	void stat_values_changed();
	void health_changed();
	void max_health_changed();
	void mana_changed();
	void max_mana_changed();
	void craft_changed();
	void max_craft_changed();
	void armor_class_bonus_changed();
	void species_armor_class_bonuses_changed();
	void to_hit_bonus_changed();
	void damage_bonus_changed();
	void range_changed();
	void movement_changed();
	void initiative_bonus_changed();
	void saving_throw_bonuses_changed();
	void skill_trainings_changed();
	void traits_changed();
	void scripted_modifiers_changed();
	void ruler_changed();
	void office_changed();
	void military_unit_changed();
	void civilian_unit_changed();
	void spells_changed();
	void recipes_changed();
	void wealth_changed();
	void items_changed();
	void unequipped_items_changed();
	void equipped_items_changed();
	void equipped_item_changed(const metternich::item_slot *slot, const int slot_index);
	void status_effect_durations_changed();

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
	int challenge_rating = 0; //the challenge rating for defeating the character in combat
	int caster_level = 0;
	const metternich::bloodline *bloodline = nullptr;
	int bloodline_strength = 0;
	int reputation = 0;
	data_entry_map<character_stat, int> stat_values;
	data_entry_map<character_attribute, std::map<character_modifier_type, std::vector<int>>> attribute_modifiers;
	int hit_dice_count = 0;
	int health = 0;
	int max_health = 0;
	std::map<dice, std::vector<int>> hit_dice_roll_results;
	int health_bonus_per_hit_dice = 0;
	int mana = 0;
	int max_mana = 0;
	int craft = 0;
	int max_craft = 0;
	int armor_class_bonus = 0;
	data_entry_map<species, int> species_armor_class_bonuses; //armor class bonuses when attacked by certain species
	int to_hit_bonus = 0;
	int damage_bonus = 0;
	int range = 1;
	int movement = 0;
	int initiative_bonus = 0;
	data_entry_map<saving_throw_type, int> saving_throw_bonuses;
	data_entry_map<skill, int> skill_trainings;
	data_entry_map<trait, int> trait_counts;
	data_entry_map<trait_type, std::vector<const trait *>> trait_choices;
	scripted_character_modifier_map<int> scripted_modifiers;
	const metternich::office *office = nullptr;
	metternich::military_unit *military_unit = nullptr;
	metternich::civilian_unit *civilian_unit = nullptr;
	std::vector<const spell *> spells; //spells that the character has learned
	std::vector<const spell *> item_spells; //spells granted by items, but which the character hasn't learned per se
	std::vector<const recipe *> recipes; //recipes that the character has learned
	int64_t wealth = 0;
	std::vector<qunique_ptr<item>> items;
	data_entry_map<item_slot, std::vector<item *>> equipped_items;
	std::map<military_unit_stat, centesimal_int> commanded_military_unit_stat_modifiers;
	military_unit_type_map<std::map<military_unit_stat, centesimal_int>> commanded_military_unit_type_stat_modifiers;
	std::vector<const trait *> target_traits;
	data_entry_map<status_effect, std::chrono::seconds> status_effect_durations;
	domain_set ruled_domains; //domains that this character has ever ruled
	domain_set reigned_domains; //domains that this character has ever ruled with a regnal number
	std::set<const flag *> flags;
};

}
