#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

enum class divine_rank;

class pantheon final : public named_data_entry, public data_type<pantheon>
{
	Q_OBJECT

public:
	static constexpr const char class_identifier[] = "pantheon";
	static constexpr const char property_class_identifier[] = "metternich::pantheon*";
	static constexpr const char database_folder[] = "pantheons";

	explicit pantheon(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	std::string_view get_divine_rank_name(const divine_rank rank) const;

signals:
	void changed();

private:
	std::map<divine_rank, std::string> divine_rank_names;
};

}
