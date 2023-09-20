#pragma once

#include "database/data_entry_history.h"

Q_MOC_INCLUDE("country/country.h")
Q_MOC_INCLUDE("map/province.h")

namespace metternich {

class character;
class country;
class province;

class character_history final : public data_entry_history
{
	Q_OBJECT

	Q_PROPERTY(metternich::country* country MEMBER country)
	Q_PROPERTY(metternich::province* deployment_province MEMBER deployment_province)

public:
	explicit character_history(const metternich::character *character) : character(character)
	{
	}

	const metternich::country *get_country() const
	{
		return this->country;
	}

	const metternich::province *get_deployment_province() const
	{
		return this->deployment_province;
	}

private:
	const metternich::character *character = nullptr;
	metternich::country *country = nullptr;
	metternich::province *deployment_province = nullptr;
};

}
