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

	Q_PROPERTY(metternich::country* country MEMBER country)
	Q_PROPERTY(metternich::province* deployment_province MEMBER deployment_province)
	Q_PROPERTY(const metternich::site* deployment_site MEMBER deployment_site)

public:
	explicit character_history(const metternich::character *character) : character(character)
	{
	}

	const metternich::country *get_country() const
	{
		return this->country;
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
	metternich::country *country = nullptr;
	province *deployment_province = nullptr;
	const site *deployment_site = nullptr;
};

}
