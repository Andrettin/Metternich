#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class population_unit;

template <typename scope_type>
class condition;

template <typename scope_type>
class factor;

class ideology final : public named_data_entry, public data_type<ideology>
{
	Q_OBJECT

	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "ideology";
	static constexpr const char property_class_identifier[] = "metternich::ideology*";
	static constexpr const char database_folder[] = "ideologies";

	explicit ideology(const std::string &identifier);
	~ideology();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const QColor &get_color() const
	{
		return this->color;
	}

	const condition<population_unit> *get_conditions() const
	{
		return this->conditions.get();
	}

	const factor<population_unit> *get_weight_factor() const
	{
		return this->weight_factor.get();
	}

signals:
	void changed();

private:
	QColor color;
	std::unique_ptr<const condition<population_unit>> conditions;
	std::unique_ptr<const factor<population_unit>> weight_factor;
};

}
