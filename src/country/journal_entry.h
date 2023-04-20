#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class country;
class portrait;

template <typename scope_type>
class condition;

template <typename scope_type>
class effect_list;

class journal_entry final : public named_data_entry, public data_type<journal_entry>
{
	Q_OBJECT

	Q_PROPERTY(metternich::portrait* portrait MEMBER portrait NOTIFY changed)
	Q_PROPERTY(std::string description MEMBER description NOTIFY changed)

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

	const condition<country> *get_preconditions() const
	{
		return this->preconditions.get();
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

	const condition<country> *get_completion_conditions() const
	{
		return this->completion_conditions.get();
	}

	const condition<country> *get_failure_conditions() const
	{
		return this->failure_conditions.get();
	}

	const effect_list<const country> *get_completion_effects() const
	{
		return this->completion_effects.get();
	}

	const effect_list<const country> *get_failure_effects() const
	{
		return this->failure_effects.get();
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
};

}
