#pragma once

#include "country/culture_container.h"
#include "country/idea.h"
#include "database/data_entry_container.h"
#include "database/data_type.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("religion/pantheon.h")

namespace metternich {

class character;
class cultural_group;
class pantheon;
class religion;

class deity final : public idea, public data_type<deity>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::pantheon* pantheon MEMBER pantheon READ get_pantheon NOTIFY changed)
	Q_PROPERTY(bool major MEMBER major READ is_major NOTIFY changed)
	Q_PROPERTY(const metternich::character* character READ get_character CONSTANT)

public:
	static constexpr const char class_identifier[] = "deity";
	static constexpr const char property_class_identifier[] = "metternich::deity*";
	static constexpr const char database_folder[] = "deities";

	static deity *add(const std::string &identifier, const metternich::data_module *data_module);

	explicit deity(const std::string &identifier);
	~deity();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	virtual idea_type get_idea_type() const override;

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

	bool is_major() const
	{
		return this->major;
	}

	const std::vector<const religion *> &get_religions() const
	{
		return this->religions;
	}

	bool can_be_worshiped() const
	{
		return !this->get_traits().empty();
	}

	const metternich::character *get_character() const
	{
		return this->character;
	}

signals:
	void changed();

private:
	const metternich::pantheon *pantheon = nullptr;
	bool major = false;
	std::vector<const religion *> religions;
	metternich::character *character = nullptr;
	culture_map<std::string> cultural_names;
	data_entry_map<cultural_group, std::string> cultural_group_names;
};

}
