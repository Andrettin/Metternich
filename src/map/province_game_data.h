#pragma once

namespace metternich {

class province;

class province_game_data final
{
public:
	explicit province_game_data(const province *province) : province(province)
	{
	}

private:
	const metternich::province *province = nullptr;
};

}
