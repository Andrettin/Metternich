#pragma once

#include "country/culture_container.h"
#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("religion/pantheon.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class character;
class cultural_group;
class deity_trait;
class pantheon;
class portrait;
class religion;
class technology;

template <typename scope_type>
class and_condition;

class deity final : public named_data_entry, public data_type<deity>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::pantheon* pantheon MEMBER pantheon READ get_pantheon NOTIFY changed)
	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(bool major MEMBER major READ is_major NOTIFY changed)
	Q_PROPERTY(const metternich::character* character READ get_character CONSTANT)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)

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

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	bool is_major() const
	{
		return this->major;
	}

	const std::vector<const religion *> &get_religions() const
	{
		return this->religions;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

	bool can_be_worshiped() const
	{
		return !this->get_traits().empty();
	}

	const metternich::character *get_character() const
	{
		return this->character;
	}

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::vector<const deity_trait *> &get_traits() const
	{
		return this->traits;
	}

	std::string get_modifier_string(const country *country) const;

	Q_INVOKABLE QString get_modifier_qstring(const country *country) const
	{
		return QString::fromStdString(this->get_modifier_string(country));
	}

	void apply_modifier(const country *country, const int multiplier) const;
	void apply_trait_modifier(const deity_trait *trait, const country *country, const int multiplier) const;

signals:
	void changed();

private:
	const metternich::pantheon *pantheon = nullptr;
	const metternich::portrait *portrait = nullptr;
	bool major = false;
	std::vector<const religion *> religions;
	metternich::character *character = nullptr;
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	culture_map<std::string> cultural_names;
	data_entry_map<cultural_group, std::string> cultural_group_names;
	std::unique_ptr<const and_condition<country>> conditions;
	std::vector<const deity_trait *> traits;
};

}
