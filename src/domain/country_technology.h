#pragma once

#include "database/data_entry_container.h"
#include "technology/technology_container.h"
#include "util/centesimal_int.h"

Q_MOC_INCLUDE("technology/technology.h")

namespace metternich {

class domain;
class domain_game_data;
class technology_category;
class technology_subcategory;

class country_technology final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList technologies READ get_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QVariantList researchable_technologies READ get_researchable_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QVariantList future_technologies READ get_future_technologies_qvariant_list NOTIFY technologies_changed)
	Q_PROPERTY(QVariantList current_researches READ get_current_researches_qvariant_list NOTIFY current_researches_changed)

public:
	explicit country_technology(const metternich::domain *domain, const domain_game_data *game_data);
	~country_technology();

	domain_game_data *get_game_data() const;

	void do_research();

	const technology_set &get_technologies() const;
	QVariantList get_technologies_qvariant_list() const;

	Q_INVOKABLE bool has_technology(const metternich::technology *technology) const
	{
		return this->get_technologies().contains(technology);
	}

	void add_technology(const technology *technology);
	void add_technology_with_prerequisites(const technology *technology);
	void on_technology_added(const technology *technology);
	void remove_technology(const technology *technology);
	void on_technology_lost(const technology *technology);
	void check_technologies();

	bool can_gain_technology(const technology *technology) const;
	Q_INVOKABLE bool can_research_technology(const metternich::technology *technology) const;

	std::vector<const technology *> get_researchable_technologies() const;
	QVariantList get_researchable_technologies_qvariant_list() const;
	Q_INVOKABLE bool is_technology_researchable(const metternich::technology *technology) const;

	QVariantList get_future_technologies_qvariant_list() const;

	const technology_set &get_current_researches() const
	{
		return this->current_researches;
	}

	QVariantList get_current_researches_qvariant_list() const;
	Q_INVOKABLE void add_current_research(const metternich::technology *technology);
	Q_INVOKABLE void remove_current_research(const metternich::technology *technology, const bool restore_costs);
	void on_technology_researched(const technology *technology);

	data_entry_map<technology_category, const technology *> get_research_choice_map(const bool is_free) const;

	void gain_free_technology();

	void gain_free_technology(const technology *technology)
	{
		this->on_technology_researched(technology);
		--this->free_technology_count;

		if (this->free_technology_count > 0) {
			this->gain_free_technology();
		}
	}

	Q_INVOKABLE void gain_free_technology(metternich::technology *technology)
	{
		const metternich::technology *const_technology = technology;
		return this->gain_free_technology(const_technology);
	}

	void gain_free_technologies(const int count);
	void gain_technologies_known_by_others();

	const centesimal_int &get_technology_cost_modifier() const
	{
		return this->technology_cost_modifier;
	}

	void change_technology_cost_modifier(const centesimal_int &change)
	{
		this->technology_cost_modifier += change;
	}

	const centesimal_int &get_technology_category_cost_modifier(const technology_category *category) const
	{
		const auto find_iterator = this->technology_category_cost_modifiers.find(category);

		if (find_iterator != this->technology_category_cost_modifiers.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	void set_technology_category_cost_modifier(const technology_category *category, const centesimal_int &value);

	void change_technology_category_cost_modifier(const technology_category *category, const centesimal_int &value)
	{
		this->set_technology_category_cost_modifier(category, this->get_technology_category_cost_modifier(category) + value);
	}

	const centesimal_int &get_technology_subcategory_cost_modifier(const technology_subcategory *subcategory) const
	{
		const auto find_iterator = this->technology_subcategory_cost_modifiers.find(subcategory);

		if (find_iterator != this->technology_subcategory_cost_modifiers.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	void set_technology_subcategory_cost_modifier(const technology_subcategory *subcategory, const centesimal_int &value);

	void change_technology_subcategory_cost_modifier(const technology_subcategory *subcategory, const centesimal_int &value)
	{
		this->set_technology_subcategory_cost_modifier(subcategory, this->get_technology_subcategory_cost_modifier(subcategory) + value);
	}

	int get_gain_technologies_known_by_others_count() const
	{
		return this->gain_technologies_known_by_others_count;
	}

	void set_gain_technologies_known_by_others_count(const int value);

	void change_gain_technologies_known_by_others_count(const int value)
	{
		this->set_gain_technologies_known_by_others_count(this->get_gain_technologies_known_by_others_count() + value);
	}

signals:
	void technologies_changed();
	void current_researches_changed();
	void technology_researched(const technology *technology);
	void technology_lost(const technology *technology);
	void available_research_slots_changed();

private:
	const metternich::domain *domain = nullptr;
	technology_set current_researches;
	int free_technology_count = 0;
	centesimal_int technology_cost_modifier;
	data_entry_map<technology_category, centesimal_int> technology_category_cost_modifiers;
	data_entry_map<technology_subcategory, centesimal_int> technology_subcategory_cost_modifiers;
	int gain_technologies_known_by_others_count = 0;
};

}
