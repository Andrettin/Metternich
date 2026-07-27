#pragma once

namespace metternich {

class province;

class province_turn_data final : public QObject
{
	Q_OBJECT

public:
	explicit province_turn_data(const metternich::province *province);
	~province_turn_data();

private:
	const metternich::province *province = nullptr;
};

}
