#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("culture/culture.h")

namespace archimedes {
	enum class gender;
}

namespace metternich {

class culture;

class dynasty final : public named_data_entry, public data_type<dynasty>
{
	Q_OBJECT

	Q_PROPERTY(archimedes::word* name_front_compound_element MEMBER name_front_compound_element WRITE set_name_front_compound_element NOTIFY changed)
	Q_PROPERTY(archimedes::word* name_rear_compound_element MEMBER name_rear_compound_element WRITE set_name_rear_compound_element NOTIFY changed)
	Q_PROPERTY(std::string prefix MEMBER prefix NOTIFY changed)
	Q_PROPERTY(bool contracted_prefix MEMBER contracted_prefix NOTIFY changed)
	Q_PROPERTY(metternich::culture* culture MEMBER culture NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "dynasty";
	static constexpr const char property_class_identifier[] = "metternich::dynasty*";
	static constexpr const char database_folder[] = "dynasties";

	explicit dynasty(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	using named_data_entry::get_name;
	const std::string &get_name(const metternich::culture *culture, const gender gender) const;

	void set_name_front_compound_element(word *word);
	void set_name_rear_compound_element(word *word);

	const std::string &get_prefix() const
	{
		return this->prefix;
	}

	const std::string &get_prefix(const metternich::culture *culture) const;

	const culture *get_culture() const
	{
		return this->culture;
	}

	std::string get_surname(const metternich::culture *culture, const gender gender) const;

signals:
	void changed();

private:
	word *name_front_compound_element = nullptr;
	word *name_rear_compound_element = nullptr;
	data_entry_map<culture_base, std::map<gender, std::string>> cultural_names;
	std::string prefix;
	bool contracted_prefix = false;
	data_entry_map<culture_base, std::string> cultural_prefixes;
	metternich::culture *culture = nullptr;
};

}
