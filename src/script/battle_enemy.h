#pragma once

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class domain;
class item_type;
class military_unit_type;
class monster_type;
enum class combat_placement;

template <typename scope_type>
class effect_list;

class battle_enemy final
{
public:
	battle_enemy();
	explicit battle_enemy(const gsml_data &scope);
	~battle_enemy();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	const metternich::military_unit_type *get_military_unit_type() const
	{
		return this->military_unit_type;
	}

	const metternich::monster_type *get_monster_type() const
	{
		return this->monster_type;
	}

	int get_health() const
	{
		return this->health;
	}

	const combat_placement get_placement() const
	{
		return this->placement;
	}

	const QPoint &get_placement_offset() const
	{
		return this->placement_offset;
	}

	const std::vector<const item_type *> &get_items() const
	{
		return this->items;
	}

	const effect_list<const domain> *get_kill_effects() const
	{
		return this->kill_effects.get();
	}

private:
	const metternich::military_unit_type *military_unit_type = nullptr;
	const metternich::monster_type *monster_type = nullptr;
	int health = 0;
	combat_placement placement{};
	QPoint placement_offset = QPoint(0, 0);
	std::vector<const item_type *> items;
	std::unique_ptr<effect_list<const domain>> kill_effects;
};

}
