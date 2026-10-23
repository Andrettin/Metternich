#pragma once

#include "database/data_entry_container.h"
#include "infrastructure/building_type_container.h"
#include "map/site_container.h"

namespace archimedes {
	class gsml_data;
	class gsml_property;
}

namespace metternich {

class domain;
class domain_game_data;
class technology;
class technology_category;

class domain_ai final : public QObject
{
	Q_OBJECT

public:
	explicit domain_ai(const metternich::domain *domain);
	~domain_ai();

	void process_gsml_property(const gsml_property &property);
	void process_gsml_scope(const gsml_data &scope);

	gsml_data to_gsml_data() const;

	domain_game_data *get_game_data() const;

	[[nodiscard]] QCoro::Task<void> do_turn();

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

	int get_holding_building_desire_modifier(const site *holding, const building_type *building) const
	{
		const auto find_iterator = this->holding_building_desire_modifiers.find(holding);

		if (find_iterator != this->holding_building_desire_modifiers.end()) {
			const auto sub_find_iterator = find_iterator->second.find(building);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void set_holding_building_desire_modifier(const site *holding, const building_type *building, const int value)
	{
		if (value == this->get_holding_building_desire_modifier(holding, building)) {
			return;
		}

		if (value == 0) {
			this->holding_building_desire_modifiers[holding].erase(building);

			if (this->holding_building_desire_modifiers[holding].empty()) {
				this->holding_building_desire_modifiers.erase(holding);
			}
		} else {
			this->holding_building_desire_modifiers[holding][building] = value;
		}
	}

	void change_holding_building_desire_modifier(const site *holding, const building_type *building, const int value)
	{
		this->set_holding_building_desire_modifier(holding, building, this->get_holding_building_desire_modifier(holding, building) + value);
	}

private:
	const metternich::domain *domain = nullptr;
	building_type_map<int> building_desire_modifiers;
	site_map<building_type_map<int>> holding_building_desire_modifiers;
};

}
