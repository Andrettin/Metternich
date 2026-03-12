#pragma once

namespace archimedes {
	class gsml_data;
}

namespace metternich {

enum class attack_result;
enum class battle_resolution_type;

class battle_resolution_entry final
{
	battle_resolution_type attacker_resolution_type{};
	battle_resolution_type defender_resolution_type{};
	std::map<int, attack_result> results;
};

class battle_resolution_table final
{
public:
	explicit battle_resolution_table(const gsml_data &scope);

	attack_result get_result(const battle_resolution_type attacker_resolution_type, const battle_resolution_type defender_resolution_type, const int attack_defense_difference) const;

private:
	std::map<battle_resolution_type, std::map<battle_resolution_type, std::map<int, attack_result>>> results;
};

}
