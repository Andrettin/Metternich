#pragma once

#include "database/data_type.h"
#include "ui/icon_base.h"
#include "util/centesimal_int.h"

namespace metternich {

class character;

template <typename scope_type>
class and_condition;

class icon final : public icon_base, public data_type<icon>
{
	Q_OBJECT

	Q_PROPERTY(archimedes::centesimal_int scale_factor MEMBER scale_factor READ get_scale_factor NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "icon";
	static constexpr const char property_class_identifier[] = "metternich::icon*";
	static constexpr const char database_folder[] = "icons";

	static const std::vector<const icon *> &get_character_icons()
	{
		return icon::character_icons;
	}

private:
	static inline std::vector<const icon *> character_icons;

public:
	explicit icon(const std::string &identifier);
	~icon();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;

	const centesimal_int &get_scale_factor() const
	{
		return this->scale_factor;
	}

	const and_condition<character> *get_character_conditions() const
	{
		return this->character_conditions.get();
	}

	bool is_character_icon() const
	{
		return this->get_character_conditions() != nullptr;
	}

private:
	centesimal_int scale_factor = centesimal_int(1);
	std::unique_ptr<const and_condition<character>> character_conditions;
};

}
