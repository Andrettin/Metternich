#pragma once

#include "script/context.h"
#include "util/dice.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("character/character.h")

namespace metternich {

class character;
class character_reference;
class party;

template <typename scope_type>
class effect_list;

struct combat_tile final
{
	const metternich::character *character = nullptr;
};

class combat_character_info final : public QObject
{
	Q_OBJECT

		Q_PROPERTY(const metternich::character *character READ get_character CONSTANT)
		Q_PROPERTY(const QPoint tile_pos READ get_tile_pos CONSTANT)
		Q_PROPERTY(bool defender READ is_defender CONSTANT)

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

	bool is_defender() const
	{
		return this->defender;
	}

private:
	const metternich::character *character = nullptr;
	QPoint tile_pos;
	bool defender = false;
};

class combat final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList character_infos READ get_character_infos_qvariant_list NOTIFY character_infos_changed)

public:
	static constexpr int map_width = 16;
	static constexpr int map_height = 15;
	static constexpr dice initiative_dice = dice(1, 10);

	struct result final
	{
		bool attacker_victory = false;
		int64_t experience_award = 0;
	};

	explicit combat(party *attacking_party, party *defending_party);
	~combat();

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
	void remove_character_info(const character *character);

	void initialize();
	void deploy_characters(const std::vector<const character *> &characters, const QPoint &start_pos, const bool defenders);
	void start();
	void do_round();

	[[nodiscard]]
	int64_t do_party_round(metternich::party *party, metternich::party *enemy_party, const int to_hit_modifier);

	void process_result();

	combat_tile &get_tile(const QPoint &tile_pos)
	{
		return this->tiles.at(tile_pos.x()).at(tile_pos.y());
	}

	bool is_tile_attacker_escape(const QPoint &tile_pos) const
	{
		return tile_pos.x() == 0;
	}

	bool is_tile_defender_escape(const QPoint &tile_pos) const
	{
		return tile_pos.x() == combat::map_width - 1;
	}

signals:
	void character_infos_changed();

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
	std::array<std::array<combat_tile, combat::map_height>, combat::map_width> tiles;
	std::vector<qunique_ptr<combat_character_info>> character_infos;
};

}
