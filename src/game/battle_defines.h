#pragma once

#include "database/defines_base.h"
#include "util/centesimal_int.h"
#include "util/singleton.h"

namespace metternich {

enum class attack_result;
enum class battle_resolution_type;

class battle_defines final : public defines_base, public singleton<battle_defines>
{
	Q_OBJECT

	Q_PROPERTY(int battle_map_scale MEMBER battle_map_scale READ get_battle_map_scale NOTIFY changed)

public:
	battle_defines();
	~battle_defines();

	virtual std::string_view get_file_name() const override
	{
		return "battle_defines.txt";
	}

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const std::chrono::seconds &get_combat_round_duration() const
	{
		return this->combat_round_duration;
	}

	const std::chrono::seconds &get_battle_round_duration() const
	{
		return this->battle_round_duration;
	}

	int get_battle_tile_length() const
	{
		return this->battle_tile_length;
	}

	int get_battle_map_scale() const
	{
		return this->battle_map_scale;
	}

	const centesimal_int &get_military_unit_hit_points_for_hit_dice(const int hit_dice_count) const;

	const std::vector<attack_result> &get_battle_resolution_results(const battle_resolution_type attacker_resolution_type, const battle_resolution_type defender_resolution_type, const int attack_defense_difference) const;

signals:
	void changed();

private:
	std::chrono::seconds combat_round_duration{};
	std::chrono::seconds battle_round_duration{};
	int battle_tile_length = 0; //the length of each side of a battle tile, in inches
	int battle_map_scale = 1; //the scale multiplier applied to the battle map (not graphical scaling!)
	std::map<int, centesimal_int> military_unit_hit_points_per_hit_dice;
	std::map<battle_resolution_type, std::map<battle_resolution_type, std::map<int, std::vector<attack_result>>>> battle_resolution_results;
};

}
