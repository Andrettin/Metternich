#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class icon;
class site;
class skill;

template <typename scope_type>
class modifier;

class site_attribute final : public named_data_entry, public data_type<site_attribute>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::icon *tiny_icon MEMBER tiny_icon READ get_tiny_icon NOTIFY changed)

public:
	static constexpr const char class_identifier[] = "site_attribute";
	static constexpr const char property_class_identifier[] = "metternich::site_attribute*";
	static constexpr const char database_folder[] = "site_attributes";

	explicit site_attribute(const std::string &identifier);
	~site_attribute();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void check() const override;

	const metternich::icon *get_tiny_icon() const
	{
		return this->tiny_icon;
	}

	bool affects_skill(const skill *skill) const;

	const modifier<const site> *get_value_modifier(const int value) const
	{
		const auto find_iterator = this->value_modifiers.find(value);
		if (find_iterator != this->value_modifiers.end()) {
			return find_iterator->second.get();
		}

		return nullptr;
	}

signals:
	void changed();

private:
	const metternich::icon *tiny_icon = nullptr;
	std::vector<const skill *> affected_skills; //character skills which are affected by this attribute (i.e. the attribute's value is applied as a modifier when a character uses the skill in the site)
	std::map<int, std::unique_ptr<modifier<const site>>> value_modifiers; //the site modifiers applied for each value; these are cumulative
};

}
