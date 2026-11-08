#pragma once

namespace metternich {

class domain;
class province;

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

private:
	metternich::province *province = nullptr;
};

}
