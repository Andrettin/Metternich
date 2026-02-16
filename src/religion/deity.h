#pragma once

#include "culture/culture_container.h"
#include "domain/idea.h"
#include "database/data_entry_container.h"
#include "database/data_type.h"

Q_MOC_INCLUDE("character/character.h")
Q_MOC_INCLUDE("religion/pantheon.h")

namespace metternich {

class character;
class cultural_group;
class divine_domain;
class pantheon;
class religion;
enum class divine_rank;

class deity final : public idea, public data_type<deity>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::pantheon* pantheon MEMBER pantheon READ get_pantheon NOTIFY changed)
	Q_PROPERTY(int divine_level MEMBER divine_level READ get_divine_level NOTIFY changed)
	Q_PROPERTY(bool apotheosis MEMBER apotheosis READ is_apotheotic NOTIFY changed)
	Q_PROPERTY(const metternich::character* character READ get_character CONSTANT)

public:
	static constexpr const char class_identifier[] = "deity";
	static constexpr const char property_class_identifier[] = "metternich::deity*";
	static constexpr const char database_folder[] = "deities";

	static const std::set<std::string> database_dependencies;

	static deity *add(const std::string &identifier, const metternich::data_module *data_module);

	explicit deity(const std::string &identifier);
	~deity();

	virtual void process_gsml_property(const gsml_property &property) override;
	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	virtual idea_type get_idea_type() const override;

	virtual const std::string &get_cultural_name(const culture *culture) const override;
	virtual const std::string &get_cultural_name(const cultural_group *cultural_group) const override;

	const metternich::pantheon *get_pantheon() const
	{
		return this->pantheon;
	}

	int get_divine_level() const
	{
		return this->divine_level;
	}

	divine_rank get_divine_rank() const;
	std::string_view get_divine_rank_name() const;

	bool is_apotheotic() const
	{
		return this->apotheosis;
	}

	const std::vector<const religion *> &get_religions() const
	{
		return this->religions;
	}

	const metternich::character *get_character() const
	{
		return this->character;
	}

	const std::vector<const divine_domain *> &get_major_domains() const
	{
		return this->major_domains;
	}

	const std::vector<const divine_domain *> &get_minor_domains() const
	{
		return this->minor_domains;
	}

	virtual bool is_available_for_country_slot(const domain *domain, const idea_slot *slot) const override;

signals:
	void changed();

private:
	const metternich::pantheon *pantheon = nullptr;
	int divine_level = 0;
	bool apotheosis = false;
	std::vector<const religion *> religions;
	metternich::character *character = nullptr;
	std::vector<const divine_domain *> major_domains;
	std::vector<const divine_domain *> minor_domains;
	culture_map<std::string> cultural_names;
	data_entry_map<cultural_group, std::string> cultural_group_names;
};

}
