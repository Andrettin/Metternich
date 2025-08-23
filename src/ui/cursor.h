#pragma once

#include "database/data_entry.h"
#include "database/data_type.h"

namespace metternich {

class renderer;
enum class cursor_type;

class cursor final : public data_entry, public data_type<cursor>
{
	Q_OBJECT

	Q_PROPERTY(std::filesystem::path filepath MEMBER filepath WRITE set_filepath)
	Q_PROPERTY(QPoint hot_pos MEMBER hot_pos READ get_hot_pos)

public:
	static constexpr const char class_identifier[] = "cursor";
	static constexpr const char property_class_identifier[] = "metternich::cursor*";
	static constexpr const char database_folder[] = "cursors";

	static void clear();

	static cursor *get_current_cursor()
	{
		return cursor::current_cursor;
	}

	static QCoro::Task<void> set_current_cursor(cursor *cursor);

private:
	static inline cursor *current_cursor = nullptr;

public:
	explicit cursor(const std::string &identifier);
	~cursor();

	virtual void initialize() override;

	const std::filesystem::path &get_filepath() const
	{
		return this->filepath;
	}

	void set_filepath(const std::filesystem::path &filepath);

	const QImage &get_image() const
	{
		return this->image;
	}

	QCoro::Task<void> load_image();

	const QPoint &get_hot_pos() const
	{
		return this->hot_pos;
	}

private:
	std::filesystem::path filepath;
	QImage image;
	QPoint hot_pos = QPoint(0, 0);
};

}
