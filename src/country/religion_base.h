#pragma once

#include "database/data_entry_container.h"
#include "database/named_data_entry.h"

namespace archimedes {
	enum class gender;
}

namespace metternich {

class government_group;
class government_type;
class office;
enum class country_tier;

class religion_base : public named_data_entry
{
	Q_OBJECT

public:
	using government_variant = std::variant<const government_type *, const government_group *>;
	using title_name_map = std::map<government_variant, std::map<country_tier, std::string>>;
	using office_title_name_map = data_entry_map<office, std::map<government_variant, std::map<country_tier, std::map<gender, std::string>>>>;

	explicit religion_base(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const std::string &get_title_name(const government_type *government_type, const country_tier tier) const;
	const std::string &get_office_title_name(const office *office, const government_type *government_type, const country_tier tier, const gender gender) const;


signals:
	void changed();

private:
	title_name_map title_names;
	office_title_name_map office_title_names;
};

}
