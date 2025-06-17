#pragma once

#include "infrastructure/building_type_container.h"
#include "map/site_container.h"

namespace metternich {

class country;
class country_game_data;

class country_ai final : public QObject
{
	Q_OBJECT

public:
	explicit country_ai(const metternich::country *country);
	~country_ai();

	country_game_data *get_game_data() const;

	void do_turn();

	void choose_current_research();
	const technology *get_research_choice(const data_entry_map<technology_category, const technology *> &research_choice_map) const;

	void appoint_ideas();
	void appoint_office_holders();

	void assign_trade_orders();

	int get_building_desire_modifier(const building_type *building) const
	{
		const auto find_iterator = this->building_desire_modifiers.find(building);

		if (find_iterator != this->building_desire_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_building_desire_modifier(const building_type *building, const int value)
	{
		if (value == this->get_building_desire_modifier(building)) {
			return;
		}

		if (value == 0) {
			this->building_desire_modifiers.erase(building);
		} else {
			this->building_desire_modifiers[building] = value;
		}
	}

	void change_building_desire_modifier(const building_type *building, const int value)
	{
		this->set_building_desire_modifier(building, this->get_building_desire_modifier(building) + value);
	}

	int get_settlement_building_desire_modifier(const site *settlement, const building_type *building) const
	{
		const auto find_iterator = this->settlement_building_desire_modifiers.find(settlement);

		if (find_iterator != this->settlement_building_desire_modifiers.end()) {
			const auto sub_find_iterator = find_iterator->second.find(building);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void set_settlement_building_desire_modifier(const site *settlement, const building_type *building, const int value)
	{
		if (value == this->get_settlement_building_desire_modifier(settlement, building)) {
			return;
		}

		if (value == 0) {
			this->settlement_building_desire_modifiers[settlement].erase(building);

			if (this->settlement_building_desire_modifiers[settlement].empty()) {
				this->settlement_building_desire_modifiers.erase(settlement);
			}
		} else {
			this->settlement_building_desire_modifiers[settlement][building] = value;
		}
	}

	void change_settlement_building_desire_modifier(const site *settlement, const building_type *building, const int value)
	{
		this->set_settlement_building_desire_modifier(settlement, building, this->get_settlement_building_desire_modifier(settlement, building) + value);
	}

private:
	const metternich::country *country = nullptr;
	building_type_map<int> building_desire_modifiers;
	site_map<building_type_map<int>> settlement_building_desire_modifiers;
};

}
