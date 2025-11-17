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

	Q_PROPERTY(bool tribal MEMBER tribal READ is_tribal NOTIFY changed)
	Q_PROPERTY(bool clade MEMBER clade READ is_clade NOTIFY changed)
	Q_PROPERTY(bool religious MEMBER religious READ is_religious NOTIFY changed)

public:
	using title_name_map = std::map<country_tier, std::string>;
	using site_title_name_map = std::map<int, std::string>;
	using office_title_name_map = data_entry_map<office, std::map<country_tier, std::map<gender, std::string>>>;

	static constexpr const char class_identifier[] = "government_group";
	static constexpr const char property_class_identifier[] = "metternich::government_group*";
	static constexpr const char database_folder[] = "government_groups";

	explicit government_group(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;

	bool is_tribal() const
	{
		return this->tribal;
	}

	bool is_clade() const
	{
		return this->clade;
	}

	bool is_religious() const
	{
		return this->religious;
	}

	const std::string &get_title_name(const country_tier tier) const;
	const std::string &get_site_title_name(const int tier) const;
	const std::string &get_office_title_name(const office *office, const country_tier tier, const gender gender) const;

signals:
	void changed();

private:
	bool tribal = false; //tribal countries do not need to be declared war upon to be attacked, nor is war necessary for them to attack others
	bool clade = false; //clade countries cannot attack non-clade countries, and do not need war declaration to be attacked
	bool religious = false; //religious government types use the title names from their religion, rather than culture
	title_name_map title_names;
	site_title_name_map site_title_names;
	office_title_name_map office_title_names;
};

}
