#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace archimedes {
	enum class gender;
}

namespace metternich {

enum class country_tier;

class government_group final : public named_data_entry, public data_type<government_group>
{
	Q_OBJECT

public:
	using title_name_map = std::map<country_tier, std::string>;
	using ruler_title_name_map = std::map<country_tier, std::map<gender, std::string>>;

	static constexpr const char class_identifier[] = "government_group";
	static constexpr const char property_class_identifier[] = "metternich::government_group*";
	static constexpr const char database_folder[] = "government_groups";

	static void process_title_names(title_name_map &title_names, const gsml_data &scope);
	static void process_ruler_title_names(ruler_title_name_map &ruler_title_names, const gsml_data &scope);
	static void process_ruler_title_name_scope(std::map<gender, std::string> &ruler_title_names, const gsml_data &scope);

	explicit government_group(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	const std::string &get_title_name(const country_tier tier) const;
	const std::string &get_ruler_title_name(const country_tier tier, const gender gender) const;

signals:
	void changed();

private:
	title_name_map title_names;
	ruler_title_name_map ruler_title_names;
};

}
