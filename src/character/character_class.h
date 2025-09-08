#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/dice.h"

Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("unit/civilian_unit_class.h")

namespace metternich {

class civilian_unit_class;
class country;
class technology;
enum class character_attribute;
enum class military_unit_category;
enum class starting_age_category;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class character_class final : public named_data_entry, public data_type<character_class>
{
	Q_OBJECT

	Q_PROPERTY(metternich::character_attribute attribute MEMBER attribute NOTIFY changed)
	Q_PROPERTY(metternich::military_unit_category military_unit_category MEMBER military_unit_category READ get_military_unit_category NOTIFY changed)
	Q_PROPERTY(const metternich::civilian_unit_class* civilian_unit_class MEMBER civilian_unit_class READ get_civilian_unit_class NOTIFY changed)
	Q_PROPERTY(int max_level MEMBER max_level READ get_max_level NOTIFY changed)
	Q_PROPERTY(archimedes::dice hit_dice MEMBER hit_dice READ get_hit_dice NOTIFY changed)
	Q_PROPERTY(archimedes::dice damage_dice MEMBER damage_dice READ get_damage_dice NOTIFY changed)
	Q_PROPERTY(int armor_class MEMBER armor_class READ get_armor_class NOTIFY changed)
	Q_PROPERTY(metternich::starting_age_category starting_age_category MEMBER starting_age_category READ get_starting_age_category NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "character_class";
	static constexpr const char property_class_identifier[] = "metternich::character_class*";
	static constexpr const char database_folder[] = "character_classes";

	explicit character_class(const std::string &identifier);
	~character_class();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	character_attribute get_attribute() const
	{
		return this->attribute;
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
		return this->max_level;
	}

	const dice &get_hit_dice() const
	{
		return this->hit_dice;
	}

	const dice &get_damage_dice() const
	{
		return this->damage_dice;
	}

	int get_armor_class() const
	{
		return this->armor_class;
	}

	metternich::starting_age_category get_starting_age_category() const
	{
		return this->starting_age_category;
	}

	technology *get_required_technology() const
	{
		return this->required_technology;
	}

	technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

	const modifier<const character> *get_level_modifier(const int level) const
	{
		const auto find_iterator = this->level_modifiers.find(level);
		if (find_iterator != this->level_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	const effect_list<const character> *get_level_effects(const int level) const
	{
		const auto find_iterator = this->level_effects.find(level);
		if (find_iterator != this->level_effects.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

	std::string get_level_effects_string(const int level, const metternich::character *character) const;

signals:
	void changed();

private:
	character_attribute attribute;
	metternich::military_unit_category military_unit_category;
	const metternich::civilian_unit_class *civilian_unit_class = nullptr;
	int max_level = 0;
	dice hit_dice;
	dice damage_dice;
	int armor_class = 0;
	metternich::starting_age_category starting_age_category{};
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	std::map<int, std::unique_ptr<const modifier<const character>>> level_modifiers;
	std::map<int, std::unique_ptr<const effect_list<const character>>> level_effects;
};

}
