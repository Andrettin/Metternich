#pragma once

#include "database/data_entry_history.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("country/office.h")
Q_MOC_INCLUDE("map/province.h")
Q_MOC_INCLUDE("map/site.h")

namespace metternich {

class character;
class country;
class office;
class province;
class site;

class character_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* country MEMBER country)
	Q_PROPERTY(const metternich::office* office MEMBER office READ get_office)
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

	const metternich::office *get_office() const
	{
		return this->office;
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
	const metternich::office *office = nullptr;
	province *deployment_province = nullptr;
	const site *deployment_site = nullptr;
};

}
