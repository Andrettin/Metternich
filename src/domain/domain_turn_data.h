#pragma once

#include "map/province_container.h"
#include "util/decimillesimal_int.h"
#include "unit/military_unit_type_container.h"
#include "util/qunique_ptr.h"

namespace metternich {

class domain;
class expense_transaction;
class income_transaction;
class technology;
enum class diplomacy_state;
enum class diplomatic_map_mode;
enum class expense_transaction_type;
enum class income_transaction_type;

class domain_turn_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList income_transactions READ get_income_transactions_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList expense_transactions READ get_expense_transactions_qvariant_list CONSTANT)

public:
	using transaction_object_variant = std::variant<std::nullptr_t, const commodity *, const population_type *>;

	explicit domain_turn_data(const metternich::domain *domain);
	~domain_turn_data();

	const decimillesimal_int &get_total_income() const
	{
		return this->total_income;
	}

	Q_INVOKABLE QString get_total_income_qstring() const;

	const decimillesimal_int &get_total_expense() const
	{
		return this->total_expense;
	}

	Q_INVOKABLE QString get_total_expense_qstring() const;
	Q_INVOKABLE QString get_total_balance_qstring() const;

	QVariantList get_income_transactions_qvariant_list() const;
	QVariantList get_expense_transactions_qvariant_list() const;

	void add_income_transaction(const income_transaction_type transaction_type, const decimillesimal_int &amount, const transaction_object_variant &object = nullptr, const decimillesimal_int &object_quantity = decimillesimal_int(0), const metternich::domain *other_domain = nullptr);
	void add_expense_transaction(const expense_transaction_type transaction_type, const decimillesimal_int &amount, const transaction_object_variant &object = nullptr, const decimillesimal_int &object_quantity = decimillesimal_int(0), const metternich::domain *other_domain = nullptr);

	const military_unit_type_map<int> &get_disbanded_military_units() const
	{
		return this->disbanded_military_units;
	}

	void add_disbanded_military_unit(const military_unit_type *military_unit_type)
	{
		++this->disbanded_military_units[military_unit_type];
	}

	const province_map<std::vector<const technology *>> &get_province_spread_technologies() const
	{
		return this->province_spread_technologies;
	}

	void add_province_spread_technology(const province *province, const technology *technology)
	{
		this->province_spread_technologies[province].push_back(technology);
	}

	bool is_diplomatic_map_dirty() const
	{
		return this->diplomatic_map_dirty;
	}

	void set_diplomatic_map_dirty(const bool value)
	{
		this->diplomatic_map_dirty = value;
	}

	bool is_realm_diplomatic_map_dirty() const
	{
		return this->realm_diplomatic_map_dirty;
	}

	void set_realm_diplomatic_map_dirty(const bool value)
	{
		this->realm_diplomatic_map_dirty = value;
	}

	const std::set<diplomatic_map_mode> &get_dirty_diplomatic_map_modes() const
	{
		return this->dirty_diplomatic_map_modes;
	}

	void set_diplomatic_map_mode_dirty(const diplomatic_map_mode mode)
	{
		this->dirty_diplomatic_map_modes.insert(mode);
	}

	const std::set<diplomacy_state> &get_dirty_diplomatic_map_diplomacy_states() const
	{
		return this->dirty_diplomatic_map_diplomacy_states;
	}

	void set_diplomatic_map_diplomacy_state_dirty(const diplomacy_state state)
	{
		this->dirty_diplomatic_map_diplomacy_states.insert(state);
	}

private:
	const metternich::domain *domain = nullptr;
	decimillesimal_int total_income;
	decimillesimal_int total_expense;
	std::vector<qunique_ptr<income_transaction>> income_transactions;
	std::vector<qunique_ptr<expense_transaction>> expense_transactions;
	military_unit_type_map<int> disbanded_military_units;
	province_map<std::vector<const technology *>> province_spread_technologies;
	bool diplomatic_map_dirty = false;
	bool realm_diplomatic_map_dirty = false;
	std::set<diplomatic_map_mode> dirty_diplomatic_map_modes;
	std::set<diplomacy_state> dirty_diplomatic_map_diplomacy_states;
};

}
