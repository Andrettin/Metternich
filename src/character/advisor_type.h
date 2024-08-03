#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("technology/technology.h")

namespace metternich {

class country;
class technology;
enum class advisor_category;

template <typename scope_type>
class effect_list;

template <typename scope_type>
class modifier;

class advisor_type final : public named_data_entry, public data_type<advisor_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::advisor_category category MEMBER category NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)
	Q_PROPERTY(metternich::technology* obsolescence_technology MEMBER obsolescence_technology NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "advisor_type";
	static constexpr const char property_class_identifier[] = "metternich::advisor_type*";
	static constexpr const char database_folder[] = "advisor_types";

	explicit advisor_type(const std::string &identifier);
	~advisor_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	advisor_category get_category() const
	{
		return this->category;
	}

	technology *get_required_technology() const
	{
		return this->required_technology;
	}

	technology *get_obsolescence_technology() const
	{
		return this->obsolescence_technology;
	}

	const metternich::modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	const metternich::modifier<const country> *get_scaled_modifier() const
	{
		return this->scaled_modifier.get();
	}

	const effect_list<const country> *get_effects() const
	{
		return this->effects.get();
	}

signals:
	void changed();

private:
	advisor_category category;
	technology *required_technology = nullptr;
	technology *obsolescence_technology = nullptr;
	std::unique_ptr<metternich::modifier<const country>> modifier;
	std::unique_ptr<metternich::modifier<const country>> scaled_modifier;
	std::unique_ptr<const effect_list<const country>> effects;
};

}
