#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("technology/technology_category.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;
class technology;
class technology_category;

class technology_subcategory final : public named_data_entry, public data_type<technology_subcategory>
{
	Q_OBJECT

	Q_PROPERTY(metternich::technology_category* category MEMBER category NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(QVariantList technologies READ get_technologies_qvariant_list NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "technology_subcategory";
	static constexpr const char property_class_identifier[] = "metternich::technology_subcategory*";
	static constexpr const char database_folder[] = "technology_subcategories";

	explicit technology_subcategory(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void initialize() override;
	virtual void check() const override;

	const technology_category *get_category() const
	{
		return this->category;
	}

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const std::vector<const technology *> &get_technologies() const
	{
		return this->technologies;
	}

	QVariantList get_technologies_qvariant_list() const;
	void add_technology(const technology *technology);

signals:
	void changed();

private:
	technology_category *category = nullptr;
	const metternich::icon *icon = nullptr;
	std::vector<const technology *> technologies;
};

}
