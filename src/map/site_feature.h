#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;
class site;

template <typename scope_type>
class factor;

template <typename scope_type>
class modifier;

class site_feature final : public named_data_entry, public data_type<site_feature>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon *icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(bool resource MEMBER resource READ is_resource NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "site_feature";
	static constexpr const char property_class_identifier[] = "metternich::site_feature*";
	static constexpr const char database_folder[] = "site_features";

	explicit site_feature(const std::string &identifier);
	~site_feature();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	bool is_resource() const
	{
		return this->resource;
	}

	const modifier<const site> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(const metternich::site *site) const;

	const factor<site> *get_weight_factor() const
	{
		return this->weight_factor.get();
	}

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	bool resource = false;
	std::unique_ptr<metternich::modifier<const site>> modifier;
	std::unique_ptr<factor<site>> weight_factor;
};

}
