#pragma once

namespace metternich {

class province;
enum class province_map_mode;

class province_turn_data final : public QObject
{
	Q_OBJECT

public:
	explicit province_turn_data(const metternich::province *province);
	~province_turn_data();

	bool is_province_map_dirty() const
	{
		return this->province_map_dirty;
	}

	void set_province_map_dirty(const bool value)
	{
		this->province_map_dirty = value;
	}

	const std::set<province_map_mode> &get_dirty_province_map_modes() const
	{
		return this->dirty_province_map_modes;
	}

	void set_province_map_mode_dirty(const province_map_mode mode)
	{
		this->dirty_province_map_modes.insert(mode);
	}

private:
	const metternich::province *province = nullptr;
	bool province_map_dirty = false;
	std::set<province_map_mode> dirty_province_map_modes;
};

}
