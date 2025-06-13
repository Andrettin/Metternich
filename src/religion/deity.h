#pragma once

#include "country/culture_container.h"
#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("religion/pantheon.h")

namespace metternich {

class character;
class cultural_group;
class divine_domain_base;
class pantheon;
class religion;

class deity final : public named_data_entry, public data_type<deity>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::pantheon* pantheon MEMBER pantheon READ get_pantheon NOTIFY changed)
	Q_PROPERTY(const metternich::character* character READ get_character CONSTANT)
	Q_PROPERTY(bool major MEMBER major READ is_major NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "deity";
	static constexpr const char property_class_identifier[] = "metternich::deity*";
	static constexpr const char database_folder[] = "deities";

	static deity *add(const std::string &identifier, const metternich::data_module *data_module);

	explicit deity(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const std::string &get_cultural_name(const culture *culture) const;
	const std::string &get_cultural_name(const cultural_group *cultural_group) const;

	Q_INVOKABLE QString get_cultural_name_qstring(const metternich::culture *culture) const
	{
		return QString::fromStdString(this->get_cultural_name(culture));
	}

	const metternich::pantheon *get_pantheon() const
	{
		return this->pantheon;
	}

	const std::vector<const religion *> &get_religions() const
	{
		return this->religions;
	}

	const metternich::character *get_character() const
	{
		return this->character;
	}

	bool is_major() const
	{
		return this->major;
	}

signals:
	void changed();

private:
	const metternich::pantheon *pantheon = nullptr;
	std::vector<const religion *> religions;
	metternich::character *character = nullptr;
	bool major = false;
	culture_map<std::string> cultural_names;
	data_entry_map<cultural_group, std::string> cultural_group_names;
};

}
