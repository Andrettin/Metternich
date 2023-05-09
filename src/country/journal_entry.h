#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class building_type;
class country;
class portrait;
class province;
class site;

template <typename scope_type>
class condition;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class journal_entry final : public named_data_entry, public data_type<journal_entry>
{
	Q_OBJECT

	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(QString description READ get_description_qstring NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "journal_entry";
	static constexpr const char property_class_identifier[] = "metternich::journal_entry*";
	static constexpr const char database_folder[] = "journal_entries";

	explicit journal_entry(const std::string &identifier);
	~journal_entry();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::portrait *get_portrait() const
	{
		return this->portrait;
	}

	const std::string &get_description() const
	{
		return this->description;
	}

	Q_INVOKABLE void set_description(const std::string &description)
	{
		this->description = description;
	}

	QString get_description_qstring() const
	{
		return QString::fromStdString(this->get_description());
	}

	const condition<country> *get_preconditions() const
	{
		return this->preconditions.get();
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	bool check_completion_conditions(const country *country) const;
	Q_INVOKABLE QString get_completion_conditions_string() const;

	const condition<country> *get_failure_conditions() const
	{
		return this->failure_conditions.get();
	}

	Q_INVOKABLE QString get_failure_conditions_string() const;

	const effect_list<const country> *get_completion_effects() const
	{
		return this->completion_effects.get();
	}

	Q_INVOKABLE QString get_completion_effects_string(metternich::country *country) const;

	const effect_list<const country> *get_failure_effects() const
	{
		return this->failure_effects.get();
	}

	Q_INVOKABLE QString get_failure_effects_string(metternich::country *country) const;

	const modifier<const country> *get_active_modifier() const
	{
		return this->active_modifier.get();
	}

	const modifier<const country> *get_completion_modifier() const
	{
		return this->completion_modifier.get();
	}

signals:
	void changed();

private:
	metternich::portrait *portrait = nullptr;
	std::string description;
	std::unique_ptr<const condition<country>> preconditions;
	std::unique_ptr<const condition<country>> conditions;
	std::unique_ptr<const condition<country>> completion_conditions;
	std::unique_ptr<const condition<country>> failure_conditions;
	std::unique_ptr<const effect_list<const country>> completion_effects;
	std::unique_ptr<const effect_list<const country>> failure_effects;
	std::unique_ptr<const modifier<const country>> active_modifier;
	std::unique_ptr<const modifier<const country>> completion_modifier;
	std::vector<const province *> owned_provinces;
	std::vector<const site *> owned_sites;
	std::vector<const building_type *> built_buildings;
};

}
