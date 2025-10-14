#pragma once

namespace metternich {

class party;

class combat final : public QObject
{
	Q_OBJECT

public:
	struct result final
	{
		bool attacker_victory = false;
		int64_t experience_award = 0;
	};

	explicit combat(party *attacking_party, party *defending_party);

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

	[[nodiscard]]
	result run();

	[[nodiscard]]
	int64_t do_party_round(metternich::party *party, metternich::party *enemy_party, const int to_hit_modifier);

signals:

private:
	party *attacking_party = nullptr;
	party *defending_party = nullptr;
	bool surprise = false;
	int attacker_to_hit_modifier = 0;
	int defender_to_hit_modifier = 0;
};

}
