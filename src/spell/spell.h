#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("sound/sound.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class arcane_school;
class character_class;
class divine_domain;
class icon;
class item_type;
class sound;
enum class attack_result;
enum class spell_target;

template <typename scope_type>
class effect_list;

class spell final : public named_data_entry, public data_type<spell>
{
	Q_OBJECT

	Q_PROPERTY(int level MEMBER level READ get_level NOTIFY changed)
	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(metternich::spell_target target MEMBER target READ get_target NOTIFY changed)
	Q_PROPERTY(metternich::spell_target battle_target MEMBER battle_target READ get_battle_target NOTIFY changed)
	Q_PROPERTY(int mana_cost MEMBER mana_cost NOTIFY changed)
	Q_PROPERTY(int range MEMBER range READ get_range NOTIFY changed)
	Q_PROPERTY(int casting_time_initiative_modifier MEMBER casting_time_initiative_modifier READ get_casting_time_initiative_modifier NOTIFY changed)
	Q_PROPERTY(int battle_range MEMBER battle_range READ get_battle_range NOTIFY changed)
	Q_PROPERTY(attack_result battle_result MEMBER battle_result READ get_battle_result NOTIFY changed)
	Q_PROPERTY(bool to_hit_check MEMBER to_hit_check READ requires_to_hit_check NOTIFY changed)
	Q_PROPERTY(const metternich::sound* sound MEMBER sound READ get_sound NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "spell";
	static constexpr const char property_class_identifier[] = "metternich::spell*";
	static constexpr const char database_folder[] = "spells";

	explicit spell(const std::string &identifier);
	~spell();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	int get_level() const
	{
		return this->level;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const int get_price() const
	{
		return this->price;
	}

	spell_target get_target() const
	{
		return this->target;
	}

	spell_target get_battle_target() const
	{
		return this->battle_target;
	}

	Q_INVOKABLE int get_mana_cost() const;

	int get_range() const
	{
		return this->range;
	}

	int get_casting_time_initiative_modifier() const
	{
		return this->casting_time_initiative_modifier;
	}

	int get_battle_range() const
	{
		return this->battle_range;
	}

	attack_result get_battle_result() const
	{
		return this->battle_result;
	}

	bool requires_to_hit_check() const
	{
		return this->to_hit_check;
	}

	const std::vector<const arcane_school *> &get_arcane_schools() const
	{
		return this->arcane_schools;
	}

	const std::vector<const divine_domain *> &get_divine_domains() const
	{
		return this->divine_domains;
	}

	const std::vector<const character_class *> &get_character_classes() const
	{
		return this->character_classes;
	}

	bool is_available_for_character_class(const character_class *character_class) const;

	const std::vector<const item_type *> &get_material_components() const
	{
		return this->material_components;
	}

	bool is_combat_spell() const;
	bool is_battle_spell() const;

	const effect_list<const character> *get_target_effects() const
	{
		return this->target_effects.get();
	}

	Q_INVOKABLE QString get_combat_effects_string(const metternich::character *caster) const;
	Q_INVOKABLE QString get_battle_effects_string() const;

	const metternich::sound *get_sound() const
	{
		return this->sound;
	}

signals:
	void changed();

private:
	int level = 0;
	metternich::icon *icon = nullptr;
	int price = 0;
	spell_target target{};
	spell_target battle_target{};
	int mana_cost = 0;
	int range = 0;
	int casting_time_initiative_modifier = 0;
	int battle_range = 0;
	attack_result battle_result{};
	bool to_hit_check = false;
	std::vector<const arcane_school *> arcane_schools;
	std::vector<const divine_domain *> divine_domains;
	std::vector<const character_class *> character_classes;
	std::vector<const item_type *> material_components;
	std::unique_ptr<const effect_list<const character>> target_effects;
	const metternich::sound *sound = nullptr;
};

}
