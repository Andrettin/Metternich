#pragma once

namespace metternich {

class country;
class province;

class province_game_data final
{
public:
	explicit province_game_data(const province *province) : province(province)
	{
	}

	const country *get_owner() const
	{
		return this->owner;
	}

	void set_owner(const country *country);

private:
	const metternich::province *province = nullptr;
	const country *owner = nullptr;
};

}
