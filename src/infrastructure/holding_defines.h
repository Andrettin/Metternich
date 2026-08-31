#pragma once

#include "database/defines_base.h"
#include "economy/commodity_container.h"
#include "util/centesimal_int.h"
#include "util/singleton.h"

namespace metternich {

enum class construction_type;
class holding_defines final : public defines_base, public singleton<holding_defines>
{
	Q_OBJECT

public:
	holding_defines();
	~holding_defines();

	virtual std::string_view get_file_name() const override
	{
		return "holding_defines.txt";
	}

	virtual const std::set<std::string> &get_database_dependencies() const override;

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const centesimal_int &get_consumption_for_holding_level(const int holding_level) const;

	const commodity_map<int64_t> &get_construction_level_commodity_costs(const construction_type construction_type) const
	{
		const auto find_iterator = this->construction_level_commodity_costs.find(construction_type);
		if (find_iterator != this->construction_level_commodity_costs.end()) {
			return find_iterator->second;
		}

		static const commodity_map<int64_t> empty_map;
		return empty_map;
	}

	const commodity_map<int64_t> &get_construction_level_commodity_costs_per_level(const construction_type construction_type) const
	{
		const auto find_iterator = this->construction_level_commodity_costs_per_level.find(construction_type);
		if (find_iterator != this->construction_level_commodity_costs_per_level.end()) {
			return find_iterator->second;
		}

		static const commodity_map<int64_t> empty_map;
		return empty_map;
	}

signals:
	void changed();

private:
	std::map<int, centesimal_int> consumption_per_holding_level;
	std::map<construction_type, commodity_map<int64_t>> construction_level_commodity_costs;
	std::map<construction_type, commodity_map<int64_t>> construction_level_commodity_costs_per_level;
};

}
