#pragma once

#include "economy/commodity_container.h"
#include "unit/military_unit_type_container.h"
#include "unit/promotion_container.h"
#include "util/centesimal_int.h"
#include "util/qunique_ptr.h"

Q_MOC_INCLUDE("unit/military_unit_type.h")

namespace metternich {

class army;
class country;
class country_game_data;
class military_unit;
class military_unit_type;
enum class military_unit_category;
enum class military_unit_stat;

class country_military final : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QVariantList leaders READ get_leaders_qvariant_list NOTIFY leaders_changed)

public:
	static constexpr int base_deployment_limit = 10;

	explicit country_military(const metternich::country *country);
	~country_military();

	country_game_data *get_game_data() const;

	void do_military_unit_recruitment();

	const std::vector<const character *> &get_leaders() const
	{
		return this->leaders;
	}

	QVariantList get_leaders_qvariant_list() const;
	void check_leaders();
	void add_leader(const character *leader);
	void remove_leader(const character *leader);
	void clear_leaders();

	const std::vector<qunique_ptr<military_unit>> &get_military_units() const
	{
		return this->military_units;
	}

	bool create_military_unit(const military_unit_type *military_unit_type, const province *deployment_province, const phenotype *phenotype, const std::vector<const promotion *> &promotions);
	void add_military_unit(qunique_ptr<military_unit> &&military_unit);
	void remove_military_unit(military_unit *military_unit);

	int get_military_unit_type_cost_modifier(const military_unit_type *military_unit_type) const;
	commodity_map<int> get_military_unit_type_commodity_costs(const military_unit_type *military_unit_type, const int quantity) const;
	Q_INVOKABLE QVariantList get_military_unit_type_commodity_costs_qvariant_list(const metternich::military_unit_type *military_unit_type, const int quantity) const;

	const military_unit_type *get_best_military_unit_category_type(const military_unit_category category, const culture *culture) const;
	Q_INVOKABLE const metternich::military_unit_type *get_best_military_unit_category_type(const metternich::military_unit_category category) const;

	const std::vector<qunique_ptr<army>> &get_armies() const
	{
		return this->armies;
	}

	void add_army(qunique_ptr<army> &&army);
	void remove_army(army *army);
	void clear_armies();

	int get_deployment_limit() const
	{
		return this->deployment_limit;
	}

	void change_deployment_limit(const int change)
	{
		this->deployment_limit += change;
	}

	const std::map<military_unit_stat, centesimal_int> &get_military_unit_type_stat_modifiers(const military_unit_type *type) const
	{
		const auto find_iterator = this->military_unit_type_stat_modifiers.find(type);

		if (find_iterator != this->military_unit_type_stat_modifiers.end()) {
			return find_iterator->second;
		}

		static const std::map<military_unit_stat, centesimal_int> empty_map;
		return empty_map;
	}

	const centesimal_int &get_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat) const
	{
		const auto find_iterator = this->military_unit_type_stat_modifiers.find(type);

		if (find_iterator != this->military_unit_type_stat_modifiers.end()) {
			const auto sub_find_iterator = find_iterator->second.find(stat);

			if (sub_find_iterator != find_iterator->second.end()) {
				return sub_find_iterator->second;
			}
		}

		static constexpr centesimal_int zero;
		return zero;
	}

	void set_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &value);

	void change_military_unit_type_stat_modifier(const military_unit_type *type, const military_unit_stat stat, const centesimal_int &change)
	{
		this->set_military_unit_type_stat_modifier(type, stat, this->get_military_unit_type_stat_modifier(type, stat) + change);
	}

	int get_infantry_cost_modifier() const
	{
		return this->infantry_cost_modifier;
	}

	void change_infantry_cost_modifier(const int change)
	{
		this->infantry_cost_modifier += change;
	}

	int get_cavalry_cost_modifier() const
	{
		return this->cavalry_cost_modifier;
	}

	void change_cavalry_cost_modifier(const int change)
	{
		this->cavalry_cost_modifier += change;
	}

	int get_artillery_cost_modifier() const
	{
		return this->artillery_cost_modifier;
	}

	void change_artillery_cost_modifier(const int change)
	{
		this->artillery_cost_modifier += change;
	}

	int get_warship_cost_modifier() const
	{
		return this->warship_cost_modifier;
	}

	void change_warship_cost_modifier(const int change)
	{
		this->warship_cost_modifier += change;
	}

	int get_unit_upgrade_cost_modifier() const
	{
		return this->unit_upgrade_cost_modifier;
	}

	void change_unit_upgrade_cost_modifier(const int change)
	{
		this->unit_upgrade_cost_modifier += change;
	}

	int get_leader_cost_modifier() const
	{
		return this->leader_cost_modifier;
	}

	void change_leader_cost_modifier(const int change)
	{
		this->leader_cost_modifier += change;
	}

	const promotion_map<int> &get_free_infantry_promotion_counts() const
	{
		return this->free_infantry_promotion_counts;
	}

	int get_free_infantry_promotion_count(const promotion *promotion) const
	{
		const auto find_iterator = this->free_infantry_promotion_counts.find(promotion);

		if (find_iterator != this->free_infantry_promotion_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_infantry_promotion_count(const promotion *promotion, const int value);

	void change_free_infantry_promotion_count(const promotion *promotion, const int value)
	{
		this->set_free_infantry_promotion_count(promotion, this->get_free_infantry_promotion_count(promotion) + value);
	}

	const promotion_map<int> &get_free_cavalry_promotion_counts() const
	{
		return this->free_cavalry_promotion_counts;
	}

	int get_free_cavalry_promotion_count(const promotion *promotion) const
	{
		const auto find_iterator = this->free_cavalry_promotion_counts.find(promotion);

		if (find_iterator != this->free_cavalry_promotion_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_cavalry_promotion_count(const promotion *promotion, const int value);

	void change_free_cavalry_promotion_count(const promotion *promotion, const int value)
	{
		this->set_free_cavalry_promotion_count(promotion, this->get_free_cavalry_promotion_count(promotion) + value);
	}

	const promotion_map<int> &get_free_artillery_promotion_counts() const
	{
		return this->free_artillery_promotion_counts;
	}

	int get_free_artillery_promotion_count(const promotion *promotion) const
	{
		const auto find_iterator = this->free_artillery_promotion_counts.find(promotion);

		if (find_iterator != this->free_artillery_promotion_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_artillery_promotion_count(const promotion *promotion, const int value);

	void change_free_artillery_promotion_count(const promotion *promotion, const int value)
	{
		this->set_free_artillery_promotion_count(promotion, this->get_free_artillery_promotion_count(promotion) + value);
	}

	const promotion_map<int> &get_free_warship_promotion_counts() const
	{
		return this->free_warship_promotion_counts;
	}

	int get_free_warship_promotion_count(const promotion *promotion) const
	{
		const auto find_iterator = this->free_warship_promotion_counts.find(promotion);

		if (find_iterator != this->free_warship_promotion_counts.end()) {
			return find_iterator->second;
		}

		return 0;
	}

	void set_free_warship_promotion_count(const promotion *promotion, const int value);

	void change_free_warship_promotion_count(const promotion *promotion, const int value)
	{
		this->set_free_warship_promotion_count(promotion, this->get_free_warship_promotion_count(promotion) + value);
	}

signals:
	void leaders_changed();
	void leader_recruited(const character *leader);
	void military_units_changed();

private:
	const metternich::country *country = nullptr;
	std::vector<const character *> leaders;
	std::vector<qunique_ptr<military_unit>> military_units;
	std::vector<qunique_ptr<army>> armies;
	int deployment_limit = country_military::base_deployment_limit;
	military_unit_type_map<std::map<military_unit_stat, centesimal_int>> military_unit_type_stat_modifiers;
	int infantry_cost_modifier = 0;
	int cavalry_cost_modifier = 0;
	int artillery_cost_modifier = 0;
	int warship_cost_modifier = 0;
	int unit_upgrade_cost_modifier = 0;
	int leader_cost_modifier = 0;
	promotion_map<int> free_infantry_promotion_counts;
	promotion_map<int> free_cavalry_promotion_counts;
	promotion_map<int> free_artillery_promotion_counts;
	promotion_map<int> free_warship_promotion_counts;
};

}
