#pragma once

#include "database/data_entry_container.h"
#include "database/data_type.h"
#include "database/named_data_entry.h"
#include "domain/law_group_container.h"

Q_MOC_INCLUDE("domain/government_group.h")
Q_MOC_INCLUDE("technology/technology.h")
Q_MOC_INCLUDE("ui/icon.h")

namespace archimedes {
	enum class gender;
}

namespace metternich {

class character_class;
class government_group;
class law;
class law_group;
class office;
class technology;
enum class domain_tier;

template <typename scope_type>
class and_condition;

template <typename scope_type>
class modifier;

class government_type final : public named_data_entry, public data_type<government_type>
{
	Q_OBJECT

	Q_PROPERTY(const metternich::government_group* group MEMBER group READ get_group NOTIFY changed)
	Q_PROPERTY(const metternich::icon* icon MEMBER icon READ get_icon NOTIFY changed)
	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	using government_variant = std::variant<const government_type *, const government_group *>;
	using title_name_map = std::map<domain_tier, std::string>;
	using site_title_name_map = std::map<int, std::string>;
	using office_title_inner_name_map = std::map<domain_tier, std::map<gender, std::string>>;
	using office_title_name_map = data_entry_map<office, office_title_inner_name_map>;

	static constexpr const char class_identifier[] = "government_type";
	static constexpr const char property_class_identifier[] = "metternich::government_type*";
	static constexpr const char database_folder[] = "government_types";

	static void process_title_name_scope(std::map<government_variant, title_name_map> &title_names, const gsml_data &scope);
	static void process_title_name_scope(title_name_map &title_names, const gsml_data &scope);
	static void process_site_title_name_scope(std::map<government_variant, site_title_name_map> &title_names, const gsml_data &scope);
	static void process_site_title_name_scope(site_title_name_map &title_names, const gsml_data &scope);
	static void process_office_title_name_scope(data_entry_map<office, std::map<government_variant, office_title_inner_name_map>> &office_title_names, const gsml_data &scope);
	static void process_office_title_name_scope(office_title_name_map &office_title_names, const gsml_data &scope);
	static void process_office_title_name_scope(std::map<government_variant, office_title_inner_name_map> &office_title_names, const gsml_data &scope);
	static void process_office_title_name_scope(office_title_inner_name_map &office_title_names, const gsml_data &scope);
	static void process_office_title_name_scope(std::map<gender, std::string> &office_title_names, const gsml_data &scope);

	explicit government_type(const std::string &identifier);
	~government_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const std::string &get_title_name(const domain_tier tier) const;
	const std::string &get_site_title_name(const int tier) const;
	const std::string &get_office_title_name(const office *office, const domain_tier tier, const gender gender) const;

	const government_group *get_group() const
	{
		return this->group;
	}

	const icon *get_icon() const
	{
		return this->icon;
	}

	const std::vector<const law *> &get_forbidden_laws() const
	{
		return this->forbidden_laws;
	}

	const law_group_map<const law *> &get_default_laws() const
	{
		return this->default_laws;
	}

	const law *get_default_law(const law_group *group) const
	{
		const auto find_iterator = this->default_laws.find(group);

		if (find_iterator != this->default_laws.end()) {
			return find_iterator->second;
		}

		return nullptr;
	}

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const and_condition<domain> *get_conditions() const
	{
		return this->conditions.get();
	}

	const modifier<const domain> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(const metternich::domain *country) const;

	const std::vector<const character_class *> &get_ruler_character_classes() const
	{
		return this->ruler_character_classes;
	}

signals:
	void changed();

private:
	const government_group *group = nullptr;
	const metternich::icon *icon = nullptr;
	std::vector<const law *> forbidden_laws;
	law_group_map<const law *> default_laws;
	technology *required_technology = nullptr;
	std::unique_ptr<const and_condition<domain>> conditions;
	std::unique_ptr<const modifier<const domain>> modifier;
	std::vector<const character_class *> ruler_character_classes;
	title_name_map title_names;
	site_title_name_map site_title_names;
	office_title_name_map office_title_names;
};

}
