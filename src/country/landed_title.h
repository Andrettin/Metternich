#pragma once

#include "database/data_type.h"
#include "database/named_data_entry.h"

namespace metternich {

class country;
class site;
enum class landed_title_tier;

class landed_title final : public named_data_entry, public data_type<landed_title>
{
	Q_OBJECT

	Q_PROPERTY(metternich::landed_title_tier default_tier MEMBER default_tier READ get_default_tier)
	Q_PROPERTY(metternich::landed_title_tier min_tier MEMBER min_tier READ get_min_tier)
	Q_PROPERTY(metternich::landed_title_tier max_tier MEMBER max_tier READ get_max_tier)

public:
	static constexpr const char class_identifier[] = "landed_title";
	static constexpr const char property_class_identifier[] = "metternich::landed_title*";
	static constexpr const char database_folder[] = "landed_titles";

	static const std::set<std::string> database_dependencies;

	explicit landed_title(const std::string &identifier);

	virtual void initialize() override;
	virtual void check() const override;

	const metternich::country *get_country() const
	{
		return this->country;
	}

	void set_country(const metternich::country *country)
	{
		this->country = country;
	}

	const metternich::site *get_site() const
	{
		return this->site;
	}

	void set_site(const metternich::site *site);

	landed_title_tier get_default_tier() const
	{
		return this->default_tier;
	}

	landed_title_tier get_min_tier() const
	{
		return this->min_tier;
	}

	landed_title_tier get_max_tier() const
	{
		return this->max_tier;
	}

private:
	const metternich::country *country = nullptr;
	const metternich::site *site = nullptr;
	landed_title_tier default_tier;
	landed_title_tier min_tier;
	landed_title_tier max_tier;
};

}