#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/dice.h"

Q_MOC_INCLUDE("character/level_bonus_table.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("unit/civilian_unit_class.h")

namespace metternich {

class character_attribute;
class civilian_unit_class;
class domain;
class item_type;
class level_bonus_table;
class species;
class technology;
enum class military_unit_category;
enum class starting_age_category;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class character_class final : public named_data_entry, public data_type<character_class>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::character_class* base_class MEMBER base_class READ get_base_class NOTIFY changed)
	Q_PROPERTY(const metternich::character_attribute* attribute MEMBER attribute READ get_attribute NOTIFY changed)
	Q_PROPERTY(metternich::military_unit_category military_unit_category MEMBER military_unit_category READ get_military_unit_category NOTIFY changed)
	Q_PROPERTY(const metternich::civilian_unit_class* civilian_unit_class MEMBER civilian_unit_class READ get_civilian_unit_class NOTIFY changed)
	Q_PROPERTY(int max_level MEMBER max_level READ get_max_level NOTIFY changed)
	Q_PROPERTY(metternich::starting_age_category starting_age_category MEMBER starting_age_category READ get_starting_age_category NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)
	Q_PROPERTY(const metternich::level_bonus_table* to_hit_bonus_table MEMBER to_hit_bonus_table READ get_to_hit_bonus_table NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "character_class";
	static constexpr const char property_class_identifier[] = "metternich::character_class*";
	static constexpr const char database_folder[] = "character_classes";

	explicit character_class(const std::string &identifier);
	~character_class();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const character_class *get_base_class() const
	{
		return this->base_class;
	}

	const character_attribute *get_attribute() const
	{
		if (this->attribute != nullptr) {
			return this->attribute;
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_attribute();
		}

		return nullptr;
	}

	metternich::military_unit_category get_military_unit_category() const
	{
		return this->military_unit_category;
	}

	const metternich::civilian_unit_class *get_civilian_unit_class() const
	{
		return this->civilian_unit_class;
	}

	int get_max_level() const
	{
		if (this->max_level != 0) {
			return this->max_level;
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_max_level();
		}

		return 0;
	}

	metternich::starting_age_category get_starting_age_category() const;

	technology *get_required_technology() const
	{
		return this->required_technology;
	}

	technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

	const level_bonus_table *get_to_hit_bonus_table() const
	{
		if (this->to_hit_bonus_table != nullptr) {
			return this->to_hit_bonus_table;
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_to_hit_bonus_table();
		}

		return nullptr;
	}

	const level_bonus_table *get_saving_throw_bonus_table(const saving_throw_type *saving_throw_type) const
	{
		const auto find_iterator = this->saving_throw_bonus_tables.find(saving_throw_type);
		if (find_iterator != this->saving_throw_bonus_tables.end()) {
			return find_iterator->second;
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_saving_throw_bonus_table(saving_throw_type);
		}

		return nullptr;
	}

	const data_entry_set<skill> &get_class_skills() const
	{
		if (!this->class_skills.empty()) {
			return this->class_skills;
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_class_skills();
		}

		return this->class_skills;
	}

	bool has_class_skill(const skill *skill) const
	{
		return this->get_class_skills().contains(skill);
	}

	bool is_allowed_for_species(const species *species) const;
	void add_allowed_species(const species *species);

	int get_min_attribute_value(const character_attribute *attribute) const
	{
		const auto find_iterator = this->min_attribute_values.find(attribute);
		if (find_iterator != this->min_attribute_values.end()) {
			return find_iterator->second;
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_min_attribute_value(attribute);
		}

		return 0;
	}

	int get_rank_level(const std::string &rank) const
	{
		const auto find_iterator = this->rank_levels.find(rank);

		if (find_iterator != this->rank_levels.end()) {
			return find_iterator->second;
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_rank_level(rank);
		}

		throw std::runtime_error(std::format("Invalid rank for class \"{}\": \"{}\".", this->get_identifier(), rank));
	}

	int64_t get_experience_for_level(const int level) const;

	const modifier<const character> *get_level_modifier(const int level) const
	{
		const auto find_iterator = this->level_modifiers.find(level);
		if (find_iterator != this->level_modifiers.end()) {
			return find_iterator->second.get();
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_level_modifier(level);
		}

		return nullptr;
	}

	const effect_list<const character> *get_level_effects(const int level) const
	{
		const auto find_iterator = this->level_effects.find(level);
		if (find_iterator != this->level_effects.end()) {
			return find_iterator->second.get();
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_level_effects(level);
		}

		return nullptr;
	}

	std::string get_level_effects_string(const int level, const metternich::character *character) const;

	const std::vector<const item_type *> &get_starting_items() const
	{
		if (!this->starting_items.empty()) {
			return this->starting_items;
		}

		if (this->get_base_class() != nullptr) {
			return this->get_base_class()->get_starting_items();
		}

		return this->starting_items;
	}

signals:
	void changed();

private:
	const character_class *base_class = nullptr;
	const character_attribute *attribute = nullptr;
	metternich::military_unit_category military_unit_category;
	const metternich::civilian_unit_class *civilian_unit_class = nullptr;
	int max_level = 0;
	metternich::starting_age_category starting_age_category{};
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	const level_bonus_table *to_hit_bonus_table = nullptr;
	data_entry_map<saving_throw_type, const level_bonus_table *> saving_throw_bonus_tables;
	data_entry_set<skill> class_skills;
	std::vector<const species *> allowed_species;
	data_entry_map<character_attribute, int> min_attribute_values;
	std::map<std::string, int> rank_levels; //names for particular levels
	std::map<int, int64_t> experience_per_level;
	std::map<int, std::unique_ptr<modifier<const character>>> level_modifiers;
	std::map<int, std::unique_ptr<effect_list<const character>>> level_effects;
	std::vector<const item_type *> starting_items;
};

}
