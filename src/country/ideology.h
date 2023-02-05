#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class ideology final : public named_data_entry, public data_type<ideology>
{
	Q_OBJECT

	Q_PROPERTY(QColor color MEMBER color READ get_color NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "ideology";
	static constexpr const char property_class_identifier[] = "metternich::ideology*";
	static constexpr const char database_folder[] = "ideologies";

	explicit ideology(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const QColor &get_color() const
	{
		return this->color;
	}

signals:
	void changed();

private:
	QColor color;
};

}
