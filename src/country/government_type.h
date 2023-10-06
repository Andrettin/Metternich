#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

Q_MOC_INCLUDE("technology/technology.h")

namespace archimedes {
	enum class gender;
}

namespace metternich {

class technology;
enum class country_tier;

template <typename scope_type>
class modifier;

class government_type final : public named_data_entry, public data_type<government_type>
{
	Q_OBJECT

	Q_PROPERTY(metternich::technology* required_technology MEMBER required_technology NOTIFY changed)

public:
	using title_name_map = std::map<country_tier, std::string>;
	using ruler_title_name_map = std::map<country_tier, std::map<gender, std::string>>;

	static constexpr const char class_identifier[] = "government_type";
	static constexpr const char property_class_identifier[] = "metternich::government_type*";
	static constexpr const char database_folder[] = "government_types";

	static void process_title_names(title_name_map &title_names, const gsml_data &scope);
	static void process_ruler_title_names(ruler_title_name_map &ruler_title_names, const gsml_data &scope);
	static void process_ruler_title_name_scope(std::map<gender, std::string> &ruler_title_names, const gsml_data &scope);

	explicit government_type(const std::string &identifier);
	~government_type();

	virtual void process_gsml_scope(const gsml_data &scope) override;
	virtual void initialize() override;
	virtual void check() const override;

	const std::string &get_title_name(const country_tier tier) const;
	const std::string &get_ruler_title_name(const country_tier tier, const gender gender) const;

	const technology *get_required_technology() const
	{
		return this->required_technology;
	}

	const modifier<const country> *get_modifier() const
	{
		return this->modifier.get();
	}

	Q_INVOKABLE QString get_modifier_string(metternich::country *country) const;

signals:
	void changed();

private:
	technology *required_technology = nullptr;
	std::unique_ptr<const modifier<const country>> modifier;
	title_name_map title_names;
	ruler_title_name_map ruler_title_names;
};

}
