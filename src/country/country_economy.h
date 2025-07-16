#pragma once

#include "economy/commodity_container.h"
#include "util/fractional_int.h"

namespace metternich {

class country;
class country_game_data;
enum class income_transaction_type;

class country_economy final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int wealth READ get_wealth NOTIFY wealth_changed)
	Q_PROPERTY(int wealth_income READ get_wealth_income NOTIFY wealth_income_changed)
	Q_PROPERTY(int credit_limit READ get_credit_limit NOTIFY credit_limit_changed)
	Q_PROPERTY(QString inflation READ get_inflation_qstring NOTIFY inflation_changed)
	Q_PROPERTY(QVariantList available_commodities READ get_available_commodities_qvariant_list NOTIFY available_commodities_changed)
	Q_PROPERTY(QVariantList tradeable_commodities READ get_tradeable_commodities_qvariant_list NOTIFY tradeable_commodities_changed)
	Q_PROPERTY(QVariantList stored_commodities READ get_stored_commodities_qvariant_list NOTIFY stored_commodities_changed)
	Q_PROPERTY(int storage_capacity READ get_storage_capacity NOTIFY storage_capacity_changed)
	Q_PROPERTY(QVariantList commodity_inputs READ get_commodity_inputs_qvariant_list NOTIFY commodity_inputs_changed)
	Q_PROPERTY(QVariantList transportable_commodity_outputs READ get_transportable_commodity_outputs_qvariant_list NOTIFY transportable_commodity_outputs_changed)
	Q_PROPERTY(QVariantList transported_commodity_outputs READ get_transported_commodity_outputs_qvariant_list NOTIFY transported_commodity_outputs_changed)
	Q_PROPERTY(QVariantList commodity_outputs READ get_commodity_outputs_qvariant_list NOTIFY commodity_outputs_changed)
	Q_PROPERTY(QVariantList everyday_consumption READ get_everyday_consumption_qvariant_list NOTIFY everyday_consumption_changed)
	Q_PROPERTY(QVariantList luxury_consumption READ get_luxury_consumption_qvariant_list NOTIFY luxury_consumption_changed)
	Q_PROPERTY(int land_transport_capacity READ get_land_transport_capacity NOTIFY land_transport_capacity_changed)
	Q_PROPERTY(int sea_transport_capacity READ get_sea_transport_capacity NOTIFY sea_transport_capacity_changed)

public:
	explicit country_economy(const metternich::country *country);
	~country_economy();

	country_game_data *get_game_data() const;

	void do_production();
	void do_everyday_consumption();
	void do_luxury_consumption();

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

	void change_wealth_inflated(const int change)
	{
		this->change_wealth(this->get_inflated_value(change));
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

	const centesimal_int &get_inflation() const
	{
		return this->inflation;
	}

	QString get_inflation_qstring() const
	{
		return QString::fromStdString(this->get_inflation().to_string());
	}

	void set_inflation(const centesimal_int &inflation);

	void change_inflation(const centesimal_int &change)
	{
		this->set_inflation(this->get_inflation() + change);
	}

	Q_INVOKABLE int get_inflated_value(const int value) const
	{
		return (value * (centesimal_int(100) + this->get_inflation()) / 100).to_int();
	}

	const centesimal_int &get_inflation_change() const
	{
		return this->inflation_change;
	}

	void set_inflation_change(const centesimal_int &inflation_change);

	void change_inflation_change(const centesimal_int &change)
	{
		this->set_inflation_change(this->get_inflation_change() + change);
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

	const commodity_map<int> &get_commodity_inputs() const
	{
		return this->commodity_inputs;
	}

	QVariantList get_commodity_inputs_qvariant_list() const;

	int get_commodity_input(const commodity *commodity) const
	{
		const auto find_iterator = this->commodity_inputs.find(commodity);

		if (find_iterator != this->commodity_inputs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE int get_commodity_input(const QString &commodity_identifier) const;
	void change_commodity_input(const commodity *commodity, const int change);

	const commodity_map<centesimal_int> &get_transportable_commodity_outputs() const
	{
		return this->transportable_commodity_outputs;
	}

	QVariantList get_transportable_commodity_outputs_qvariant_list() const;

	const centesimal_int &get_transportable_commodity_output(const commodity *commodity) const
	{
		const auto find_iterator = this->transportable_commodity_outputs.find(commodity);

		if (find_iterator != this->transportable_commodity_outputs.end()) {
			return find_iterator->second;
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	Q_INVOKABLE int get_transportable_commodity_output(const QString &commodity_identifier) const;
	void change_transportable_commodity_output(const commodity *commodity, const centesimal_int &change);

	const commodity_map<int> &get_transported_commodity_outputs() const
	{
		return this->transported_commodity_outputs;
	}

	QVariantList get_transported_commodity_outputs_qvariant_list() const;

	Q_INVOKABLE int get_transported_commodity_output(const metternich::commodity *commodity) const
	{
		const auto find_iterator = this->transported_commodity_outputs.find(commodity);

		if (find_iterator != this->transported_commodity_outputs.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	Q_INVOKABLE void change_transported_commodity_output(const metternich::commodity *commodity, const int change);

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
		return this->get_commodity_output(commodity).to_int() - this->get_commodity_input(commodity);
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

	void assign_production();
	void decrease_wealth_consumption(const bool restore_inputs = true);
	void decrease_commodity_consumption(const commodity *commodity, const bool restore_inputs = true);

	bool produces_commodity(const commodity *commodity) const;

	int get_land_transport_capacity() const
	{
		return this->land_transport_capacity;
	}

	void set_land_transport_capacity(const int capacity);

	void change_land_transport_capacity(const int change)
	{
		this->set_land_transport_capacity(this->get_land_transport_capacity() + change);
	}

	int get_sea_transport_capacity() const
	{
		return this->sea_transport_capacity;
	}

	void set_sea_transport_capacity(const int capacity);

	void change_sea_transport_capacity(const int change)
	{
		this->set_sea_transport_capacity(this->get_sea_transport_capacity() + change);
	}

	int get_available_transport_capacity() const
	{
		const int total_capacity = this->get_land_transport_capacity() + this->get_sea_transport_capacity();
		int available_capacity = total_capacity;
		for (const auto &[commodity, transported_output] : this->get_transported_commodity_outputs()) {
			available_capacity -= transported_output;
		}
		return available_capacity;
	}

	void assign_transport_orders();

signals:
	void wealth_changed();
	void wealth_income_changed();
	void credit_limit_changed();
	void inflation_changed();
	void available_commodities_changed();
	void tradeable_commodities_changed();
	void stored_commodities_changed();
	void storage_capacity_changed();
	void commodity_inputs_changed();
	void transportable_commodity_outputs_changed();
	void transported_commodity_outputs_changed();
	void commodity_outputs_changed();
	void everyday_wealth_consumption_changed();
	void everyday_consumption_changed();
	void luxury_consumption_changed();
	void land_transport_capacity_changed();
	void sea_transport_capacity_changed();

private:
	const metternich::country *country = nullptr;
	int wealth = 0;
	int wealth_income = 0;
	int credit_limit = 0;
	centesimal_int inflation;
	centesimal_int inflation_change;
	commodity_set available_commodities;
	commodity_set tradeable_commodities;
	commodity_map<int> stored_commodities;
	int storage_capacity = 0;
	commodity_map<int> commodity_inputs;
	commodity_map<centesimal_int> transportable_commodity_outputs;
	commodity_map<int> transported_commodity_outputs;
	commodity_map<centesimal_int> commodity_outputs;
	int everyday_wealth_consumption = 0;
	commodity_map<centesimal_int> everyday_consumption;
	commodity_map<centesimal_int> luxury_consumption;
	commodity_map<decimillesimal_int> commodity_demands;
	int land_transport_capacity = 0;
	int sea_transport_capacity = 0;
};

}
