#pragma once

#include "util/centesimal_int.h"
#include "util/qunique_ptr.h"

namespace metternich {

class domain;
class expense_transaction;
class income_transaction;
enum class diplomacy_state;
enum class diplomatic_map_mode;
enum class expense_transaction_type;
enum class income_transaction_type;

class country_turn_data final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(int total_income READ get_total_income CONSTANT)
	Q_PROPERTY(int total_expense READ get_total_expense CONSTANT)
	Q_PROPERTY(QVariantList income_transactions READ get_income_transactions_qvariant_list CONSTANT)
	Q_PROPERTY(QVariantList expense_transactions READ get_expense_transactions_qvariant_list CONSTANT)

public:
	using transaction_object_variant = std::variant<std::nullptr_t, const commodity *, const population_type *>;

	explicit country_turn_data(const metternich::domain *domain);
	~country_turn_data();

	int get_total_income() const
	{
		return this->total_income;
	}

	int get_total_expense() const
	{
		return this->total_expense;
	}

	QVariantList get_income_transactions_qvariant_list() const;
	QVariantList get_expense_transactions_qvariant_list() const;

	void add_income_transaction(const income_transaction_type transaction_type, const int amount, const transaction_object_variant &object = nullptr, const int object_quantity = 0, const metternich::domain *other_domain = nullptr);
	void add_expense_transaction(const expense_transaction_type transaction_type, const int amount, const transaction_object_variant &object = nullptr, const int object_quantity = 0, const metternich::domain *other_domain = nullptr);

	bool is_diplomatic_map_dirty() const
	{
		return this->diplomatic_map_dirty;
	}

	void set_diplomatic_map_dirty(const bool value)
	{
		this->diplomatic_map_dirty = value;
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
	int total_income = 0;
	int total_expense = 0;
	std::vector<qunique_ptr<income_transaction>> income_transactions;
	std::vector<qunique_ptr<expense_transaction>> expense_transactions;
	bool diplomatic_map_dirty = false;
	std::set<diplomatic_map_mode> dirty_diplomatic_map_modes;
	std::set<diplomacy_state> dirty_diplomatic_map_diplomacy_states;
};

}
