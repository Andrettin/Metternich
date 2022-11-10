#pragma once

#include "country/culture_base.h"
#include "database/data_type.h"

namespace metternich {

class cultural_group;

class culture final : public culture_base, public data_type<culture>
{
	Q_OBJECT

	Q_PROPERTY(metternich::cultural_group* group MEMBER group NOTIFY changed)
	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "culture";
	static constexpr const char property_class_identifier[] = "metternich::culture*";
	static constexpr const char database_folder[] = "cultures";

	explicit culture(const std::string &identifier) : culture_base(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const cultural_group *get_group() const
	{
		return this->group;
	}

	const QColor &get_color() const
	{
		return this->color;
	}

	const population_type *get_population_class_type(const population_class *population_class) const;

signals:
	void changed();

private:
	cultural_group *group = nullptr;
	QColor color;
};

}
