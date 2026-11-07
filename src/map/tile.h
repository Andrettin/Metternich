#pragma once

#include "economy/commodity_container.h"

namespace metternich {

class domain;
class province;
class resource;
class site;

class tile final
{
public:
	metternich::province *get_province() const
	{
		return this->province;
	}

	void set_province(metternich::province *province)
	{
		this->province = province;
	}

	const domain *get_owner() const;

	const metternich::site *get_site() const
	{
		return this->site;
	}

	void set_site(const metternich::site *site);

private:
	metternich::province *province = nullptr;
	const metternich::site *site = nullptr;
};

}
