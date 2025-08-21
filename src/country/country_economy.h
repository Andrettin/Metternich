#pragma once

#include "country/country_container.h"
#include "economy/commodity_container.h"
#include "economy/resource_container.h"
#include "infrastructure/building_type_container.h"
#include "infrastructure/improvement_container.h"
#include "population/profession_container.h"
#include "util/centesimal_int.h"
#include "util/decimillesimal_int.h"

namespace metternich {

class country;
class country_game_data;
class employment_location;
enum class income_transaction_type;

class country_economy final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList resource_counts READ get_resource_counts_qvariant_list NOTIFY resource_counts_changed)
	Q_PROPERTY(QVariantList vassal_resource_counts READ get_vassal_resource_counts_qvariant_list NOTIFY vassal_resource_counts_changed)
	Q_PROPERTY(int wealth READ get_wealth NOTIFY wealth_changed)
	Q_PROPERTY(int wealth_income READ get_wealth_income NOTIFY wealth_income_changed)
	Q_PROPERTY(int credit_limit READ get_credit_limit NOTIFY credit_limit_changed)
	Q_PROPERTY(QVariantList available_commodities READ get_available_commodities_qvariant_list NOTIFY available_commodities_changed)
	Q_PROPERTY(QVariantList tradeable_commodities READ get_tradeable_commodities_qvariant_list NOTIFY tradeable_commodities_changed)
	Q_PROPERTY(QVariantList stored_commodities READ get_stored_commodities_qvariant_list NOTIFY stored_commodities_changed)
	Q_PROPERTY(int storage_capacity READ get_storage_capacity NOTIFY storage_capacity_changed)
	Q_PROPERTY(QVariantList commodity_inputs READ get_commodity_inputs_qvariant_list NOTIFY commodity_inputs_changed)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)
	Q_PROPERTY(QVariantList everyday_consumption READ get_everyday_consumption_qvariant_list NOTIFY everyday_consumption_changed)
	Q_PROPERTY(QVariantList luxury_consumption READ get_luxury_consumption_qvariant_list NOTIFY luxury_consumption_changed)
	Q_PROPERTY(QVariantList bids READ get_bids_qvariant_list NOTIFY bids_changed)
	Q_PROPERTY(QVariantList offers READ get_offers_qvariant_list NOTIFY offers_changed)
	Q_PROPERTY(int output_modifier READ get_output_modifier_int NOTIFY output_modifier_changed)
	Q_PROPERTY(int resource_output_modifier READ get_resource_output_modifier NOTIFY resource_output_modifier_changed)
	Q_PROPERTY(int industrial_output_modifier READ get_industrial_output_modifier NOTIFY industrial_output_modifier_changed)
	Q_PROPERTY(int throughput_modifier READ get_throughput_modifier NOTIFY throughput_modifier_changed)

public:
	explicit country_economy(const metternich::country *country, const country_game_data *game_data);
	~country_economy();

	country_game_data *get_game_data() const;

	void do_production();
	void do_everyday_consumption();
	void do_luxury_consumption();
	void do_trade(country_map<commodity_map<int>> &country_luxury_demands);

	const resource_map<int> &get_resource_counts() const
	{
		return this->resource_counts;
	}

	QVariantList get_resource_counts_qvariant_list() const;

	void change_resource_count(const resource *resource, const int change)
	{
		const int final_count = (this->resource_counts[resource] += change);

		if (final_count == 0) {
			this->resource_counts.erase(resource);
		}
	}

	const resource_map<int> &get_vassal_resource_counts() const
	{
		return this->vassal_resource_counts;
	}

	QVariantList get_vassal_resource_counts_qvariant_list() const;

	void change_vassal_resource_count(const resource *resource, const int change)
	{
		const int final_count = (this->vassal_resource_counts[resource] += change);

		if (final_count == 0) {
			this->vassal_resource_counts.erase(resource);
		}
	}

	int get_wealth() const
	{
		return this->wealth;
	}

	void set_wealth(const int wealth)
	{
		if (wealth == this->get_wealth()) {
			return;
		}

		this->wealth = wealth;

		emit wealth_changed();
	}

	void change_wealth(const int change)
	{
		this->set_wealth(this->get_wealth() + change);
	}

	void add_taxable_wealth(const int taxable_wealth, const income_transaction_type tax_income_type);

	int get_wealth_income() const
	{
		return this->wealth_income;
	}

	void set_wealth_income(const int income);

	void change_wealth_income(const int change)
	{
		this->set_wealth_income(this->get_wealth_income() + change);
	}

	int get_credit_limit() const
	{
		return this->credit_limit;
	}

	void set_credit_limit(const int credit_limit)
	{
		if (credit_limit == this->get_credit_limit()) {
			return;
		}

		this->credit_limit = credit_limit;

		emit credit_limit_changed();
	}

	void change_credit_limit(const int change)
	{
		this->set_credit_limit(this->get_credit_limit() + change);
	}

	int get_wealth_with_credit() const
	{
		return this->get_wealth() + this->get_credit_limit();
	}

	const commodity_set &get_available_commodities() const
	{
		return this->available_commodities;
	}

	QVariantList get_available_commodities_qvariant_list() const;

	void add_available_commodity(const commodity *commodity)
	{
		this->available_commodities.insert(commodity);
		emit available_commodities_changed();
	}

	void remove_available_commodity(const commodity *commodity)
	{
		this->available_commodities.erase(commodity);
		emit available_commodities_changed();
	}

	const commodity_set &get_tradeable_commodities() const
	{
		return this->tradeable_commodities;
	}

	QVariantList get_tradeable_commodities_qvariant_list() const;

	void add_tradeable_commodity(const commodity *commodity)
	{
		this->tradeable_commodities.insert(commodity);
		emit tradeable_commodities_changed();
	}

	void remove_tradeable_commodity(const commodity *commodity)
	{
		this->tradeable_commodities.erase(commodity);
		emit tradeable_commodities_changed();
	}

	bool can_trade_commodity(const commodity *commodity) const
	{
		return this->get_tradeable_commodities().contains(commodity);
	}

	const commodity_map<int> &get_stored_commodities() const
	{
		return this->stored_commodities;
	}

	QVariantList get_stored_commodities_qvariant_list() const;

	Q_INVOKABLE int get_stored_commodity(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->stored_commodities.find(commodity);

		if (find_iterator != this->stored_commodities.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_stored_commodity(const commodity *commodity, const int value);

	void change_stored_commodity(const commodity *commodity, const int value)
	{
		this->set_stored_commodity(commodity, this->get_stored_commodity(commodity) + value);
	}

	int get_stored_food() const;

	int get_storage_capacity() const
	{
		return this->storage_capacity;
	}

	void set_storage_capacity(const int capacity);

	void change_storage_capacity(const int change)
	{
		this->set_storage_capacity(this->get_storage_capacity() + change);
	}

	const commodity_map<centesimal_int> &get_commodity_inputs() const
	{
		return this->commodity_inputs;
	}

	QVariantList get_commodity_inputs_qvariant_list() const;

	const centesimal_int &get_commodity_input(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_inputs.find(commodity);

		if (find_iterator != this->commodity_inputs.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_commodity_input(const QString &commodity_identifier) const;
	void change_commodity_input(const commodity *commodity, const centesimal_int &change, const bool change_input_storage);
	bool can_change_commodity_input(const commodity *commodity, const centesimal_int &change) const;

	const commodity_map<centesimal_int> &get_commodity_outputs() const
	{
		return this->commodity_outputs;
	}

	QVariantList get_commodity_outputs_qvariant_list() const;

	const centesimal_int &get_commodity_output(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_outputs.find(commodity);

		if (find_iterator != this->commodity_outputs.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_commodity_output(const QString &commodity_identifier) const;
	void change_commodity_output(const commodity *commodity, const centesimal_int &change);

	int get_net_commodity_output(const commodity *commodity) const
	{
		return this->get_commodity_output(commodity).to_int() - this->get_commodity_input(commodity).to_int();
	}

	void calculate_site_commodity_outputs();
	void calculate_site_commodity_output(const commodity *commodity);

	int get_food_output() const;

	int get_everyday_wealth_consumption() const
	{
		return this->everyday_wealth_consumption;
	}

	void change_everyday_wealth_consumption(const int change);

	const commodity_map<centesimal_int> &get_everyday_consumption() const
	{
		return this->everyday_consumption;
	}

	QVariantList get_everyday_consumption_qvariant_list() const;

	const centesimal_int &get_everyday_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->everyday_consumption.find(commodity);

		if (find_iterator != this->everyday_consumption.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_everyday_consumption(const QString &commodity_identifier) const;
	void change_everyday_consumption(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_luxury_consumption() const
	{
		return this->luxury_consumption;
	}

	QVariantList get_luxury_consumption_qvariant_list() const;

	const centesimal_int &get_luxury_consumption(const commodity *commodity) const
	{
		const auto find_iterator = this->luxury_consumption.find(commodity);

		if (find_iterator != this->luxury_consumption.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_luxury_consumption(const QString &commodity_identifier) const;
	void change_luxury_consumption(const commodity *commodity, const centesimal_int &change);

	const commodity_map<decimillesimal_int> &get_commodity_demands() const
	{
		return this->commodity_demands;
	}

	const decimillesimal_int &get_commodity_demand(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_demands.find(commodity);

		if (find_iterator != this->commodity_demands.end()) {
			return find_iterator->second;
		}

		static const decimillesimal_int zero;
		return zero;
	}

	void change_commodity_demand(const commodity *commodity, const decimillesimal_int &change);

	employment_location * decrease_wealth_consumption(const bool restore_inputs = true);
	employment_location * decrease_commodity_consumption(const commodity *commodity, const bool restore_inputs = true);

	bool produces_commodity(const commodity *commodity) const;

	const commodity_map<int> &get_bids() const
	{
		return this->bids;
	}

	QVariantList get_bids_qvariant_list() const;

	Q_INVOKABLE int get_bid(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->bids.find(commodity);

		if (find_iterator != this->bids.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE void set_bid(const metternich::commodity *commodity, const int value);

	Q_INVOKABLE void change_bid(const metternich::commodity *commodity, const int change)
	{
		this->set_bid(commodity, this->get_bid(commodity) + change);
	}

	void clear_bids()
	{
		this->bids.clear();
		emit bids_changed();
	}

	const commodity_map<int> &get_offers() const
	{
		return this->offers;
	}

	QVariantList get_offers_qvariant_list() const;

	Q_INVOKABLE int get_offer(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->offers.find(commodity);

		if (find_iterator != this->offers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE void set_offer(const metternich::commodity *commodity, const int value);

	Q_INVOKABLE void change_offer(const metternich::commodity *commodity, const int change)
	{
		this->set_offer(commodity, this->get_offer(commodity) + change);
	}

	void clear_offers()
	{
		this->offers.clear();
		emit offers_changed();
	}

	void do_sale(const metternich::country *other_country, const commodity *commodity, const int sold_quantity, const bool state_purchase);

	const commodity_map<int> &get_commodity_needs() const
	{
		return this->commodity_needs;
	}

	int get_commodity_need(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->commodity_needs.find(commodity);

		if (find_iterator != this->commodity_needs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_commodity_need(const metternich::commodity *commodity, const int value)
	{
		if (value == this->get_commodity_need(commodity)) {
			return;
		}

		if (value == 0) {
			this->commodity_needs.erase(commodity);
		} else {
			this->commodity_needs[commodity] = value;
		}
	}

	void calculate_commodity_needs();

	const centesimal_int &get_output_modifier() const
	{
		return this->output_modifier;
	}

	int get_output_modifier_int() const
	{
		return this->get_output_modifier().to_int();
	}

	void set_output_modifier(const centesimal_int &value);

	void change_output_modifier(const centesimal_int &change)
	{
		this->set_output_modifier(this->get_output_modifier() + change);
	}

	int get_resource_output_modifier() const
	{
		return this->resource_output_modifier;
	}

	void set_resource_output_modifier(const int value);

	void change_resource_output_modifier(const int value)
	{
		this->set_resource_output_modifier(this->get_resource_output_modifier() + value);
	}

	int get_industrial_output_modifier() const
	{
		return this->industrial_output_modifier;
	}

	void set_industrial_output_modifier(const int value);

	void change_industrial_output_modifier(const int value)
	{
		this->set_industrial_output_modifier(this->get_industrial_output_modifier() + value);
	}

	const commodity_map<centesimal_int> &get_commodity_output_modifiers() const
	{
		return this->commodity_output_modifiers;
	}

	const centesimal_int &get_commodity_output_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_output_modifiers.find(commodity);

		if (find_iterator != this->commodity_output_modifiers.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_commodity_output_modifier(metternich::commodity *commodity) const
	{
		const metternich::commodity *const_commodity = commodity;
		return this->get_commodity_output_modifier(const_commodity).to_int();
	}

	void set_commodity_output_modifier(const commodity *commodity, const centesimal_int &value);

	void change_commodity_output_modifier(const commodity *commodity, const centesimal_int &change)
	{
		this->set_commodity_output_modifier(commodity, this->get_commodity_output_modifier(commodity) + change);
	}

	const commodity_map<centesimal_int> &get_capital_commodity_output_modifiers() const
	{
		return this->capital_commodity_output_modifiers;
	}

	const centesimal_int &get_capital_commodity_output_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->capital_commodity_output_modifiers.find(commodity);

		if (find_iterator != this->capital_commodity_output_modifiers.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_capital_commodity_output_modifier(const commodity *commodity, const centesimal_int &value);

	void change_capital_commodity_output_modifier(const commodity *commodity, const centesimal_int &change)
	{
		this->set_capital_commodity_output_modifier(commodity, this->get_capital_commodity_output_modifier(commodity) + change);
	}

	int get_throughput_modifier() const
	{
		return this->throughput_modifier;
	}

	void set_throughput_modifier(const int value);

	void change_throughput_modifier(const int value)
	{
		this->set_throughput_modifier(this->get_throughput_modifier() + value);
	}

	int get_commodity_throughput_modifier(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_throughput_modifiers.find(commodity);

		if (find_iterator != this->commodity_throughput_modifiers.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE int get_commodity_throughput_modifier(metternich::commodity *commodity) const
	{
		const metternich::commodity *const_commodity = commodity;
		return this->get_commodity_throughput_modifier(const_commodity);
	}

	void set_commodity_throughput_modifier(const commodity *commodity, const int value);

	void change_commodity_throughput_modifier(const commodity *commodity, const int value)
	{
		this->set_commodity_throughput_modifier(commodity, this->get_commodity_throughput_modifier(commodity) + value);
	}

	const resource_map<commodity_map<int>> &get_improved_resource_commodity_bonuses() const
	{
		return this->improved_resource_commodity_bonuses;
	}

	int get_improved_resource_commodity_bonus(const commodity *commodity, const resource *resource) const
	{
		const auto find_iterator = this->improved_resource_commodity_bonuses.find(resource);

		if (find_iterator != this->improved_resource_commodity_bonuses.end()) {
			const auto sub_find_iterator = find_iterator->second.find(commodity);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void change_improved_resource_commodity_bonus(const resource *resource, const commodity *commodity, const int change);

	const improvement_map<commodity_map<centesimal_int>> &get_improvement_commodity_bonuses() const
	{
		return this->improvement_commodity_bonuses;
	}

	const commodity_map<centesimal_int> &get_improvement_commodity_bonuses(const improvement *improvement) const
	{
		const auto find_iterator = this->improvement_commodity_bonuses.find(improvement);

		if (find_iterator != this->improvement_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const commodity_map<centesimal_int> empty_map;
		return empty_map;
	}

	const centesimal_int &get_improvement_commodity_bonus(const commodity *commodity, const improvement *improvement) const
	{
		const auto find_iterator = this->improvement_commodity_bonuses.find(improvement);

		if (find_iterator != this->improvement_commodity_bonuses.end()) {
			const auto sub_find_iterator = find_iterator->second.find(commodity);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void change_improvement_commodity_bonus(const improvement *improvement, const commodity *commodity, const centesimal_int &change);

	const building_type_map<commodity_map<int>> &get_building_commodity_bonuses() const
	{
		return this->building_commodity_bonuses;
	}

	const commodity_map<int> &get_building_commodity_bonuses(const building_type *building) const
	{
		const auto find_iterator = this->building_commodity_bonuses.find(building);

		if (find_iterator != this->building_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const commodity_map<int> empty_map;
		return empty_map;
	}

	int get_building_commodity_bonus(const commodity *commodity, const building_type *building) const
	{
		const auto find_iterator = this->building_commodity_bonuses.find(building);

		if (find_iterator != this->building_commodity_bonuses.end()) {
			const auto sub_find_iterator = find_iterator->second.find(commodity);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void change_building_commodity_bonus(const building_type *building, const commodity *commodity, const int change);

	const profession_map<commodity_map<decimillesimal_int>> &get_profession_commodity_bonuses() const
	{
		return this->profession_commodity_bonuses;
	}

	const commodity_map<decimillesimal_int> &get_profession_commodity_bonuses(const profession *profession) const
	{
		const auto find_iterator = this->profession_commodity_bonuses.find(profession);

		if (find_iterator != this->profession_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const commodity_map<decimillesimal_int> empty_map;
		return empty_map;
	}

	const decimillesimal_int &get_profession_commodity_bonus(const profession *profession, const commodity *commodity) const
	{
		const auto find_iterator = this->profession_commodity_bonuses.find(profession);

		if (find_iterator != this->profession_commodity_bonuses.end()) {
			const auto sub_find_iterator = find_iterator->second.find(commodity);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		static constexpr decimillesimal_int zero;
		return zero;
	}

	void change_profession_commodity_bonus(const profession *profession, const commodity *commodity, const decimillesimal_int &change);

	const commodity_map<std::map<int, int>> &get_commodity_bonuses_for_tile_thresholds() const
	{
		return this->commodity_bonuses_for_tile_thresholds;
	}

	int get_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold) const
	{
		const auto find_iterator = this->commodity_bonuses_for_tile_thresholds.find(commodity);

		if (find_iterator != this->commodity_bonuses_for_tile_thresholds.end()) {
			const auto sub_find_iterator = find_iterator->second.find(threshold);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		return 0;
	}

	void set_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value);

	void change_commodity_bonus_for_tile_threshold(const commodity *commodity, const int threshold, const int value)
	{
		this->set_commodity_bonus_for_tile_threshold(commodity, threshold, this->get_commodity_bonus_for_tile_threshold(commodity, threshold) + value);
	}

	const commodity_map<centesimal_int> &get_commodity_bonuses_per_population() const
	{
		return this->commodity_bonuses_per_population;
	}

	const centesimal_int &get_commodity_bonus_per_population(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_bonuses_per_population.find(commodity);

		if (find_iterator != this->commodity_bonuses_per_population.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;

		return zero;
	}

	void change_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_settlement_commodity_bonuses() const
	{
		return this->settlement_commodity_bonuses;
	}

	const centesimal_int &get_settlement_commodity_bonus(const commodity *commodity) const
	{
		const auto find_iterator = this->settlement_commodity_bonuses.find(commodity);

		if (find_iterator != this->settlement_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;

		return zero;
	}

	void change_settlement_commodity_bonus(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_capital_commodity_bonuses() const
	{
		return this->capital_commodity_bonuses;
	}

	const centesimal_int &get_capital_commodity_bonus(const commodity *commodity) const
	{
		const auto find_iterator = this->capital_commodity_bonuses.find(commodity);

		if (find_iterator != this->capital_commodity_bonuses.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;

		return zero;
	}

	void change_capital_commodity_bonus(const commodity *commodity, const centesimal_int &change);

	const commodity_map<centesimal_int> &get_capital_commodity_bonuses_per_population() const
	{
		return this->capital_commodity_bonuses_per_population;
	}

	const centesimal_int &get_capital_commodity_bonus_per_population(const commodity *commodity) const
	{
		const auto find_iterator = this->capital_commodity_bonuses_per_population.find(commodity);

		if (find_iterator != this->capital_commodity_bonuses_per_population.end()) {
			return find_iterator->second;
		}

		static const centesimal_int zero;

		return zero;
	}

	void change_capital_commodity_bonus_per_population(const commodity *commodity, const centesimal_int &change);

signals:
	void resource_counts_changed();
	void vassal_resource_counts_changed();
	void wealth_changed();
	void wealth_income_changed();
	void credit_limit_changed();
	void available_commodities_changed();
	void tradeable_commodities_changed();
	void stored_commodities_changed();
	void storage_capacity_changed();
	void commodity_inputs_changed();
	void commodity_outputs_changed();
	void everyday_wealth_consumption_changed();
	void everyday_consumption_changed();
	void luxury_consumption_changed();
	void bids_changed();
	void offers_changed();
	void output_modifier_changed();
	void resource_output_modifier_changed();
	void industrial_output_modifier_changed();
	void throughput_modifier_changed();

private:
	const metternich::country *country = nullptr;
	resource_map<int> resource_counts;
	resource_map<int> vassal_resource_counts;
	int wealth = 0;
	int wealth_income = 0;
	int credit_limit = 0;
	commodity_set available_commodities;
	commodity_set tradeable_commodities;
	commodity_map<int> stored_commodities;
	int storage_capacity = 0;
	commodity_map<centesimal_int> commodity_inputs;
	commodity_map<centesimal_int> commodity_outputs;
	int everyday_wealth_consumption = 0;
	commodity_map<centesimal_int> everyday_consumption;
	commodity_map<centesimal_int> luxury_consumption;
	commodity_map<decimillesimal_int> commodity_demands;
	commodity_map<int> bids;
	commodity_map<int> offers;
	commodity_map<int> commodity_needs;
	centesimal_int output_modifier;
	int resource_output_modifier = 0;
	int industrial_output_modifier = 0;
	commodity_map<centesimal_int> commodity_output_modifiers;
	commodity_map<centesimal_int> capital_commodity_output_modifiers;
	int throughput_modifier = 0;
	commodity_map<int> commodity_throughput_modifiers;
	resource_map<commodity_map<int>> improved_resource_commodity_bonuses;
	improvement_map<commodity_map<centesimal_int>> improvement_commodity_bonuses;
	building_type_map<commodity_map<int>> building_commodity_bonuses;
	profession_map<commodity_map<decimillesimal_int>> profession_commodity_bonuses;
	commodity_map<std::map<int, int>> commodity_bonuses_for_tile_thresholds;
	commodity_map<centesimal_int> commodity_bonuses_per_population;
	commodity_map<centesimal_int> settlement_commodity_bonuses;
	commodity_map<centesimal_int> capital_commodity_bonuses;
	commodity_map<centesimal_int> capital_commodity_bonuses_per_population;
};

}
