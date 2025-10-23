#pragma once

#include "character/character_container.h"
#include "script/context.h"
#include "util/dice.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("item/object_type.h")

namespace metternich {

class character;
class character_reference;
class combat_object;
class object_type;
class party;
class terrain_type;

template <typename scope_type>
class effect_list;

struct combat_tile final
{
	explicit combat_tile(const terrain_type *base_terrain, const terrain_type *terrain);

	bool is_occupied() const
	{
		return this->character != nullptr || this->object != nullptr;
	}

	const terrain_type *terrain = nullptr;
	short base_tile_frame = 0;
	std::array<short, 4> base_subtile_frames {};
	short tile_frame = 0;
	std::array<short, 4> subtile_frames {};
	const metternich::character *character = nullptr;
	const combat_object *object = nullptr;
};

class combat_character_info final : public QObject
{
	Q_OBJECT

		Q_PROPERTY(const metternich::character *character READ get_character CONSTANT)
		Q_PROPERTY(const QPoint tile_pos READ get_tile_pos NOTIFY pos_changed)
		Q_PROPERTY(const QPoint pixel_offset READ get_pixel_offset NOTIFY pos_changed)
		Q_PROPERTY(bool defender READ is_defender CONSTANT)
		Q_PROPERTY(int remaining_movement READ get_remaining_movement NOTIFY remaining_movement_changed)

public:
	explicit combat_character_info(const metternich::character *character, const QPoint &tile_pos, const bool defender)
		: character(character), tile_pos(tile_pos), defender(defender)
	{
	}

	const metternich::character *get_character() const
	{
		return this->character;
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
		this->pixel_offset = QPoint(0, 0);
		emit pos_changed();
	}

	const QPoint &get_pixel_offset() const
	{
		return this->pixel_offset;
	}

	void set_pixel_offset(const QPoint &pixel_offset)
	{
		if (pixel_offset == this->get_pixel_offset()) {
			return;
		}

		this->pixel_offset = pixel_offset;
		emit pos_changed();
	}

	bool is_defender() const
	{
		return this->defender;
	}

	int get_remaining_movement() const
	{
		return this->remaining_movement;
	}

	void set_remaining_movement(const int movement)
	{
		if (movement == this->get_remaining_movement()) {
			return;
		}

		this->remaining_movement = movement;
	}

	void change_remaining_movement(const int change)
	{
		this->set_remaining_movement(this->get_remaining_movement() + change);
	}

signals:
	void pos_changed();
	void remaining_movement_changed();

private:
	const metternich::character *character = nullptr;
	QPoint tile_pos;
	QPoint pixel_offset = QPoint(0, 0);
	bool defender = false;
	int remaining_movement = 0;
};

class combat_object final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(const metternich::object_type* object_type READ get_object_type CONSTANT)
	Q_PROPERTY(const QPoint tile_pos READ get_tile_pos NOTIFY tile_pos_changed)

public:
	explicit combat_object(const metternich::object_type *object_type, const effect_list<const character> *use_effects)
		: object_type(object_type), use_effects(use_effects)
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

signals:
	void tile_pos_changed();

private:
	const metternich::object_type *object_type = nullptr;
	QPoint tile_pos = QPoint(-1, -1);
	const effect_list<const character> *use_effects = nullptr;
};

class combat final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList character_infos READ get_character_infos_qvariant_list NOTIFY character_infos_changed)
	Q_PROPERTY(QVariantList objects READ get_objects_qvariant_list NOTIFY objects_changed)
	Q_PROPERTY(bool autoplay_enabled READ is_autoplay_enabled WRITE set_autoplay_enabled NOTIFY autoplay_enabled_changed)

public:
	static constexpr dice initiative_dice = dice(1, 10);

	struct result final
	{
		bool attacker_victory = false;
		int64_t experience_award = 0;
	};

	explicit combat(party *attacking_party, party *defending_party, const QSize &map_size);
	~combat();

	const QRect &get_map_rect() const
	{
		return this->map_rect;
	}

	int get_map_width() const
	{
		return this->get_map_rect().width();
	}

	int get_map_height() const
	{
		return this->get_map_rect().height();
	}

	const terrain_type *get_base_terrain() const
	{
		return this->base_terrain;
	}

	void set_base_terrain(const terrain_type *terrain);

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

	void set_attacker_retreat_allowed(const bool allowed)
	{
		this->attacker_retreat_allowed = allowed;
	}

	void set_defender_retreat_allowed(const bool allowed)
	{
		this->defender_retreat_allowed = allowed;
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

	void set_character_kill_effects(const character_map<const effect_list<const domain> *> &character_kill_effects)
	{
		this->character_kill_effects = character_kill_effects;
	}

	void set_victory_effects(const effect_list<const domain> *victory_effects)
	{
		this->victory_effects = victory_effects;
	}

	void set_defeat_effects(const effect_list<const domain> *defeat_effects)
	{
		this->defeat_effects = defeat_effects;
	}

	QVariantList get_character_infos_qvariant_list() const;
	combat_character_info *get_character_info(const character *character) const;
	void remove_character_info(const character *character);

	QVariantList get_objects_qvariant_list() const;
	void add_object(const object_type *object_type, const effect_list<const character> *use_effects);
	void remove_object(const combat_object *object);

	void initialize();
	void deploy_objects(const QPoint &start_pos);
	void deploy_characters(std::vector<const character *> characters, const QPoint &start_pos, const bool defenders);

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

	int64_t do_character_attack(const character *character, const metternich::character *enemy, party *enemy_party, const int to_hit_modifier);

	void process_result();

	combat_tile &get_tile(const QPoint &tile_pos);
	const combat_tile &get_tile(const QPoint &tile_pos) const;

	bool is_tile_attacker_escape(const QPoint &tile_pos) const
	{
		return tile_pos.x() == 0;
	}

	bool is_tile_defender_escape(const QPoint &tile_pos) const
	{
		return tile_pos.x() == (this->get_map_width() - 1);
	}

	[[nodiscard]]
	QCoro::Task<void> move_character_to(const character *character, const QPoint tile_pos);

	Q_INVOKABLE void set_target(const QPoint &tile_pos);

	bool can_current_character_move_to(const QPoint &tile_pos) const;
	bool can_current_character_retreat_at(const QPoint &tile_pos) const;
	bool is_current_character_in_enemy_range_at(const QPoint &tile_pos) const;

	bool is_autoplay_enabled() const
	{
		return this->autoplay_enabled;
	}

	void set_autoplay_enabled(const bool enabled)
	{
		this->autoplay_enabled = enabled;
		emit autoplay_enabled_changed();
	}

signals:
	void character_infos_changed();
	void objects_changed();
	void tile_character_changed(const QPoint &tile_pos);
	void tile_object_changed(const QPoint &tile_pos);
	void movable_tiles_changed();
	void autoplay_enabled_changed();

private:
	QRect map_rect;
	const terrain_type *base_terrain = nullptr;
	party *attacking_party = nullptr;
	party *defending_party = nullptr;
	bool surprise = false;
	int attacker_to_hit_modifier = 0;
	int defender_to_hit_modifier = 0;
	bool attacker_retreat_allowed = true;
	bool defender_retreat_allowed = true;
	combat::result result;
	int64_t attacker_experience_award = 0;
	int64_t defender_experience_award = 0;
	std::vector<std::shared_ptr<character_reference>> generated_characters;
	std::unique_ptr<party> generated_party;
	const domain *scope = nullptr;
	context ctx;
	character_map<const effect_list<const domain> *> character_kill_effects;
	const effect_list<const domain> *victory_effects = nullptr;
	const effect_list<const domain> *defeat_effects = nullptr;
	std::vector<combat_tile> tiles;
	character_map<qunique_ptr<combat_character_info>> character_infos;
	std::vector<qunique_ptr<combat_object>> objects;
	std::unique_ptr<QPromise<QPoint>> target_promise;
	const character *current_character = nullptr;
	bool autoplay_enabled = false;
};

}
