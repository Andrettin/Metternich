#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("ui/icon.h")

namespace metternich {

class icon;
class province;
class terrain_type;

template <typename scope_type>
class factor;

template <typename scope_type>
class modifier;

class province_feature final : public named_data_entry, public data_type<province_feature>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon *icon MEMBER icon READ get_icon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "province_feature";
	static constexpr const char property_class_identifier[] = "metternich::province_feature*";
	static constexpr const char database_folder[] = "province_features";

	explicit province_feature(const std::string &identifier);
	~province_feature();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::icon *get_icon() const
	{
		return this->icon;
	}

	const std::vector<const terrain_type *> &get_terrain_types() const
	{
		return this->terrain_types;
	}

	const modifier<const province> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(const metternich::province *province) const;

signals:
	void changed();

private:
	const metternich::icon *icon = nullptr;
	std::vector<const terrain_type *> terrain_types;
	std::unique_ptr<metternich::modifier<const province>> modifier;
};

}
