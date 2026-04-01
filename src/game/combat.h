#pragma once

#include "character/character_container.h"
#include "game/combat_base.h"
#include "script/context.h"
#include "util/dice.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("item/object_type.h")
Q_MOC_INCLUDE("item/trap_type.h")

namespace metternich {

class character;
class character_reference;
class combat;
class combat_object;
class object_type;
class party;
class province;
class trap_type;

template <typename scope_type>
class effect_list;

struct combat_tile final : public combat_tile_base
{
	explicit combat_tile(const terrain_type *base_terrain, const terrain_type *terrain);

	virtual bool is_occupied() const override
	{
		return this->character != nullptr || this->object != nullptr;
	}

	const metternich::character *character = nullptr;
	combat_object *object = nullptr;
};

class combat_character_info final : public combat_unit_info_base
{
	Q_OBJECT

public:
	explicit combat_character_info(const metternich::character *character, const bool defender);

	virtual const metternich::character *get_character() const override
	{
		return this->character;
	}

	virtual const icon *get_icon() const override;

	const effect_list<const domain> *get_kill_effects() const
	{
		return this->kill_effects;
	}

	void set_kill_effects(const effect_list<const domain> *kill_effects)
	{
		this->kill_effects = kill_effects;
	}

	virtual int get_hit_points() const override;
	virtual int get_max_hit_points() const override;
	virtual int get_range() const override;
	virtual bool is_player_unit() const override;
	virtual bool is_player_enemy() const override;

private:
	const metternich::character *character = nullptr;
	const effect_list<const domain> *kill_effects = nullptr;
};

class combat_object final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::object_type* object_type READ get_object_type CONSTANT)
	Q_PROPERTY(QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)
	Q_PROPERTY(const metternich::trap_type* trap READ get_trap NOTIFY trap_changed)
	Q_PROPERTY(bool trap_found READ get_trap_found NOTIFY trap_found_changed)

public:
	explicit combat_object(const metternich::combat *combat, const metternich::object_type *object_type, const effect_list<const character> *use_effects, const trap_type *trap, const std::string &description, const combat_placement placement, const QPoint &placement_offset)
		: combat(combat), object_type(object_type), use_effects(use_effects), trap(trap), description(description), placement(placement), placement_offset(placement_offset)
	{
	}

	const metternich::object_type *get_object_type() const
	{
		return this->object_type;
	}

	const QPoint &get_tile_pos() const
	{
		return this->tile_pos;
	}

	void set_tile_pos(const QPoint &tile_pos)
	{
		if (tile_pos == this->get_tile_pos()) {
			return;
		}

		this->tile_pos = tile_pos;
		emit tile_pos_changed();
	}

	const effect_list<const character> *get_use_effects() const
	{
		return this->use_effects;
	}

	const trap_type *get_trap() const
	{
		return this->trap;
	}

	void remove_trap()
	{
		this->trap = nullptr;
		this->set_trap_found(false);
		emit trap_changed();
	}

	bool get_trap_found() const
	{
		return this->trap_found;
	}

	void set_trap_found(const bool found)
	{
		if (found == this->get_trap_found()) {
			return;
		}

		this->trap_found = found;
		emit trap_found_changed();
	}

	Q_INVOKABLE int get_disarm_chance(const metternich::character *character) const;

	const std::string &get_description() const
	{
		return this->description;
	}

	combat_placement get_placement() const
	{
		return this->placement;
	}

	const QPoint &get_placement_offset() const
	{
		return this->placement_offset;
	}

signals:
	void tile_pos_changed();
	void trap_changed();
	void trap_found_changed();

private:
	const metternich::combat *combat = nullptr;
	const metternich::object_type *object_type = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	const effect_list<const character> *use_effects = nullptr;
	const trap_type *trap = nullptr;
	bool trap_found = false;
	std::string description;
	combat_placement placement = combat_placement::right;
	QPoint placement_offset = QPoint(0, 0);
};

class combat final : public combat_base
{
	Q_OBJECT

	Q_PROPERTY(QVariantList objects READ get_objects_qvariant_list NOTIFY objects_changed)

public:
	static constexpr dice initiative_dice = dice(1, 10);

	struct result final
	{
		bool attacker_victory = false;
		int64_t experience_award = 0;
	};

	explicit combat(party *attacking_party, party *defending_party, const QSize &map_size);
	~combat();

	virtual int get_max_range_of_units() const override;
	virtual spell_target get_spell_target(const spell *spell) const override;
	virtual int get_spell_range(const spell *spell) const override;

	void set_surprise(const bool surprise)
	{
		this->surprise = surprise;
	}

	void set_attacker_to_hit_modifier(const int modifier)
	{
		this->attacker_to_hit_modifier = modifier;
	}

	void set_defender_to_hit_modifier(const int modifier)
	{
		this->defender_to_hit_modifier = modifier;
	}

	void set_generated_characters(const std::vector<std::shared_ptr<character_reference>> &generated_characters)
	{
		this->generated_characters = generated_characters;
	}

	void set_generated_party(std::unique_ptr<party> &&generated_party);

	void set_scope(const domain *scope)
	{
		this->scope = scope;
	}

	void set_context(const context &ctx)
	{
		this->ctx = ctx;
	}

	void set_victory_effects(const effect_list<const domain> *victory_effects)
	{
		this->victory_effects = victory_effects;
	}

	void set_defeat_effects(const effect_list<const domain> *defeat_effects)
	{
		this->defeat_effects = defeat_effects;
	}

	virtual QVariantList get_unit_infos_qvariant_list() const override;
	combat_character_info *get_character_info(const character *character) const;
	void remove_character_info(const character *character);

	QVariantList get_objects_qvariant_list() const;
	void add_object(const object_type *object_type, const effect_list<const character> *use_effects, const trap_type *trap, const std::string &description, const combat_placement placement, const QPoint &placement_offset);
	void remove_object(const combat_object *object);

	void initialize();
	void deploy_objects();
	void deploy_characters(std::vector<const character *> characters, const bool defenders);

	Q_INVOKABLE QCoro::QmlTask start()
	{
		return this->start_coro();
	}

	[[nodiscard]]
	QCoro::Task<void> start_coro();

	[[nodiscard]]
	QCoro::Task<void> do_round();

	[[nodiscard]]
	QCoro::Task<int64_t> do_party_round(metternich::party *party, metternich::party *enemy_party, const int to_hit_modifier);

	const character *choose_enemy(const character *character, const std::vector<const metternich::character *> &enemies) const;
	const combat_object *choose_target_object(const character *character) const;

	bool do_to_hit_check(const character *character, const metternich::character *enemy, const int to_hit_modifier) const;

	[[nodiscard]]
	QCoro::Task<int64_t> do_character_attack(const character *character, const metternich::character *enemy, party *enemy_party, const int to_hit_modifier);

	[[nodiscard]]
	QCoro::Task<int64_t> do_character_spellcast(const character *caster, const spell *spell, const metternich::character *target, party *target_party, const int to_hit_modifier);

	[[nodiscard]]
	QCoro::Task<void> on_character_killed(const character *dead_character, party *dead_character_party, const metternich::character *killer);

	[[nodiscard]]
	QCoro::Task<void> on_character_died(const character *dead_character, party *dead_character_party);

	void process_result();

	virtual combat_tile &get_tile(const QPoint &tile_pos) override;
	virtual const combat_tile &get_tile(const QPoint &tile_pos) const override;
	virtual std::string get_tile_text(const QPoint &tile_pos) const override;
	virtual combat_unit_info_base *get_tile_unit(const QPoint &tile_pos) const override;

	virtual bool is_attacker_defeated() const override;
	virtual bool is_defender_defeated() const override;

	[[nodiscard]]
	QCoro::Task<void> move_character_to(const character *character, const QPoint tile_pos);

	virtual bool is_current_unit_in_enemy_range_at(const QPoint &tile_pos) const override;
	bool can_character_use_object(const character *character, const combat_object *object) const;
	bool can_current_character_use_object(const combat_object *object) const;

	const site *get_location() const;

signals:
	void objects_changed();

private:
	party *attacking_party = nullptr;
	party *defending_party = nullptr;
	bool surprise = false;
	int attacker_to_hit_modifier = 0;
	int defender_to_hit_modifier = 0;
	combat::result result;
	int64_t attacker_experience_award = 0;
	int64_t defender_experience_award = 0;
	std::vector<std::shared_ptr<character_reference>> generated_characters;
	std::unique_ptr<party> generated_party;
	const domain *scope = nullptr;
	context ctx;
	const effect_list<const domain> *victory_effects = nullptr;
	const effect_list<const domain> *defeat_effects = nullptr;
	std::vector<combat_tile> tiles;
	character_map<qunique_ptr<combat_character_info>> character_infos;
	std::vector<qunique_ptr<combat_object>> objects;
};

}
