#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class mythic_path final : public named_data_entry, public data_type<mythic_path>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "mythic_path";
	static constexpr const char property_class_identifier[] = "metternich::mythic_path*";
	static constexpr const char database_folder[] = "mythic_paths";

	explicit mythic_path(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	int get_rank_tier(const std::string &rank) const
	{
		const auto find_iterator = this->rank_tiers.find(rank);

		if (find_iterator != this->rank_tiers.end()) {
			return find_iterator->second;
		}

		throw std::runtime_error(std::format("Invalid rank for mythic path \"{}\": \"{}\".", this->get_identifier(), rank));
	}

	const std::string &get_tier_title_name(const int mythic_tier) const;

signals:
	void changed();

private:
	std::map<std::string, int> rank_tiers; //names for particular tiers
	std::map<int, std::string> tier_title_names; //character title names, available from particular tiers
};

}
