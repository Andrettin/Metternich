#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;
class technology;

class technology_category final : public named_data_entry, public data_type<technology_category>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(QVariantList technologies READ get_technologies_qvariant_list NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "technology_category";
	static constexpr const char property_class_identifier[] = "metternich::technology_category*";
	static constexpr const char database_folder[] = "technology_categories";

	explicit technology_category(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const std::vector<const technology *> &get_technologies() const
	{
		return this->technologies;
	}

	QVariantList get_technologies_qvariant_list() const;

	void add_technology(const technology *technology)
	{
		this->technologies.push_back(technology);
	}

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	std::vector<const technology *> technologies;
};

}
