#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;

class technology final : public named_data_entry, public data_type<technology>
{
	Q_OBJECT

	Q_PROPERTY(metternich::icon* icon MEMBER icon NOTIFY changed)
	Q_PROPERTY(QVariantList prerequisites READ get_prerequisites_qvariant_list NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "technology";
	static constexpr const char property_class_identifier[] = "metternich::technology*";
	static constexpr const char database_folder[] = "technologies";

	explicit technology(const std::string &identifier) : named_data_entry(identifier)
	{
	}

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const std::vector<const technology *> get_prerequisites() const
	{
		return this->prerequisites;
	}

	QVariantList get_prerequisites_qvariant_list() const;

	bool requires_technology(const technology *technology) const;
	int get_total_prerequisite_depth() const;

signals:
	void changed();

private:
	metternich::icon *icon = nullptr;
	std::vector<const technology *> prerequisites;
};

}
