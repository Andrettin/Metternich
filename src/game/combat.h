#pragma once

#include "script/context.h"
#include "util/dice.h"

namespace metternich {

class character_reference;
class party;

template <typename scope_type>
class effect_list;

class combat final : public QObject
{
	Q_OBJECT

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

	void start();
	void do_round();

	[[nodiscard]]
	int64_t do_party_round(metternich::party *party, metternich::party *enemy_party, const int to_hit_modifier);

	void process_result();

signals:

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
};

}
