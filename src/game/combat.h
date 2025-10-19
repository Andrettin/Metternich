#pragma once

#include "character/character_container.h"
#include "script/context.h"
#include "util/dice.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")

namespace metternich {

class character;
class character_reference;
class party;
class terrain_type;

template <typename scope_type>
class effect_list;

struct combat_tile final
{
	explicit combat_tile(const terrain_type *base_terrain, const terrain_type *terrain);

	const terrain_type *terrain = nullptr;
	short base_tile_frame = 0;
	std::array<short, 4> base_subtile_frames {};
	short tile_frame = 0;
	std::array<short, 4> subtile_frames {};
	const metternich::character *character = nullptr;
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

class combat final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList character_infos READ get_character_infos_qvariant_list NOTIFY character_infos_changed)

public:
	static constexpr int map_width = 16;
	static constexpr int map_height = 12;
	static constexpr dice initiative_dice = dice(1, 10);

	struct result final
	{
		bool attacker_victory = false;
		int64_t experience_award = 0;
	};

	explicit combat(party *attacking_party, party *defending_party);
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

	QVariantList get_character_infos_qvariant_list() const;
	combat_character_info *get_character_info(const character *character) const;
	void remove_character_info(const character *character);

	void initialize();
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

signals:
	void character_infos_changed();
	void tile_character_changed(const QPoint &tile_pos);
	void movable_tiles_changed();

private:
	QRect map_rect;
	const terrain_type *base_terrain = nullptr;
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
	std::unique_ptr<QPromise<QPoint>> target_promise;
	const character *current_character = nullptr;
};

}
