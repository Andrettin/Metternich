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
	Q_PROPERTY(bool religious MEMBER religious READ is_religious NOTIFY changed)

public:
	using title_name_map = std::map<country_tier, std::string>;
	using ruler_title_name_map = std::map<country_tier, std::map<gender, std::string>>;

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

	bool is_religious() const
	{
		return this->religious;
	}

	const std::string &get_title_name(const country_tier tier) const;
	const std::string &get_ruler_title_name(const country_tier tier, const gender gender) const;

signals:
	void changed();

private:
	bool tribal = false; //tribal countries do not need to be declared war upon to be attacked, nor is war necessary for them to attack others
	bool religious = false; //religious government types use the title names from their religion, rather than culture
	title_name_map title_names;
	ruler_title_name_map ruler_title_names;
};

}
