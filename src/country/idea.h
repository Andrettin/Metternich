#pragma once

#include "database/named_data_entry.h"

Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/portrait.h")

namespace metternich {

class country;
class portrait;
class idea_slot;
class idea_trait;
class technology;
enum class idea_type;

template <typename scope_type>
class and_condition;

class idea : public named_data_entry
{
	Q_OBJECT

	Q_PROPERTY(metternich::idea_type idea_type READ get_idea_type CONSTANT)
	Q_PROPERTY(const metternich::portrait* portrait MEMBER portrait READ get_portrait NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)

public:
	explicit idea(const std::string &identifier);
	~idea();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	virtual idea_type get_idea_type() const = 0;

	virtual const std::string &get_cultural_name(const culture *culture) const
	{
		Q_UNUSED(culture);

		return this->get_name();
	}

	virtual const std::string &get_cultural_name(const cultural_group *cultural_group) const
	{
		Q_UNUSED(cultural_group);

		return this->get_name();
	}

	Q_INVOKABLE QString get_cultural_name_qstring(const metternich::culture *culture) const
	{
		return QString::fromStdString(this->get_cultural_name(culture));
	}

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	virtual int get_skill() const
	{
		return 0;
	}

	technology *get_required_technology() const
	{
		return this->required_technology;
	}

	technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

	bool is_available() const
	{
		return !this->get_traits().empty();
	}

	virtual bool is_available_for_country_slot(const country *country, const idea_slot *slot) const;

	const and_condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const std::vector<const idea_trait *> &get_traits() const
	{
		return this->traits;
	}

	void add_trait(const idea_trait *trait)
	{
		this->traits.push_back(trait);
	}

	std::string get_modifier_string(const country *country) const;

	Q_INVOKABLE QString get_modifier_qstring(const country *country) const
	{
		return QString::fromStdString(this->get_modifier_string(country));
	}

	void apply_modifier(const country *country, const int multiplier) const;
	void apply_trait_modifier(const idea_trait *trait, const country *country, const int multiplier) const;

signals:
	void changed();

private:
	const metternich::portrait *portrait = nullptr;
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	std::unique_ptr<const and_condition<country>> conditions;
	std::vector<const idea_trait *> traits;
};

}
