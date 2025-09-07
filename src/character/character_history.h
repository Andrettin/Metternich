#pragma once

#include "database/data_entry_history.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("map/site.h")

namespace metternich {

class character;
class country;
class province;
class site;

class character_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(const metternich::country* country MEMBER country)
	Q_PROPERTY(int level MEMBER level READ get_level)
	Q_PROPERTY(const metternich::province* deployment_province MEMBER deployment_province)
	Q_PROPERTY(const metternich::site* deployment_site MEMBER deployment_site)

public:
	explicit character_history(const metternich::character *character);

	const metternich::country *get_country() const
	{
		return this->country;
	}

	int get_level() const
	{
		return this->level;
	}

	const province *get_deployment_province() const
	{
		return this->deployment_province;
	}

	const site *get_deployment_site() const
	{
		return this->deployment_site;
	}

private:
	const metternich::character *character = nullptr;
	const metternich::country *country = nullptr;
	int level = 0;
	const province *deployment_province = nullptr;
	const site *deployment_site = nullptr;
};

}
