#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "util/fractional_int.h"

namespace metternich {

class country;

template <typename scope_type>
class condition;

class country_rank final : public named_data_entry, public data_type<country_rank>
{
	Q_OBJECT

	Q_PROPERTY(int priority MEMBER priority READ get_priority NOTIFY changed)
	Q_PROPERTY(archimedes::centesimal_int average_score_threshold MEMBER average_score_threshold READ get_average_score_threshold)
	Q_PROPERTY(archimedes::centesimal_int relative_score_threshold MEMBER relative_score_threshold READ get_relative_score_threshold)

public:
	static constexpr const char class_identifier[] = "country_rank";
	static constexpr const char property_class_identifier[] = "metternich::country_rank*";
	static constexpr const char database_folder[] = "country_ranks";

	explicit country_rank(const std::string &identifier);
	~country_rank();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	int get_priority() const
	{
		return this->priority;
	}

	const centesimal_int &get_average_score_threshold() const
	{
		return this->average_score_threshold;
	}

	const centesimal_int &get_relative_score_threshold() const
	{
		return this->relative_score_threshold;
	}

	const condition<country> *get_conditions() const
	{
		return this->conditions.get();
	}

signals:
	void changed();

private:
	int priority = 0;
	centesimal_int average_score_threshold;
	centesimal_int relative_score_threshold;
	std::unique_ptr<const condition<country>> conditions;
};

}
