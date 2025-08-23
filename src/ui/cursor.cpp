#include "metternich.h"

#include "ui/cursor.h"

#include "database/database.h"
#include "database/preferences.h"
#include "util/assert_util.h"
#include "util/image_util.h"
#include "util/path_util.h"

#include "xbrz.h"

#pragma warning(push, 0)
#include <QPixmap>
#pragma warning(pop)

namespace metternich {

void cursor::clear()
{
	data_type::clear();

	cursor::current_cursor = nullptr;
}

QCoro::Task<void> cursor::set_current_cursor(cursor *cursor)
{
	if (cursor == cursor::current_cursor) {
		co_return;
	}

	cursor::current_cursor = cursor;

	if (cursor != nullptr) {
		if (cursor->get_image().isNull()) {
			co_await cursor->load_image();
		}

		const QPixmap pixmap = QPixmap::fromImage(cursor->get_image());
		const QPoint hot_pos = cursor->get_hot_pos() * preferences::get()->get_scale_factor();
		const QCursor qcursor(pixmap, hot_pos.x(), hot_pos.y());

		QMetaObject::invokeMethod(QApplication::instance(), [qcursor] {
			if (QApplication::overrideCursor() != nullptr) {
				QApplication::changeOverrideCursor(qcursor);
			} else {
				QApplication::setOverrideCursor(qcursor);
			}
		}, Qt::QueuedConnection);
	}
}

cursor::cursor(const std::string &identifier) : data_entry(identifier)
{
}


cursor::~cursor()
{
}

void cursor::initialize()
{
	assert_throw(!this->get_filepath().empty());
	assert_throw(std::filesystem::exists(this->get_filepath()));

	QTimer::singleShot(0, [this]() -> QCoro::Task<void> {
		co_await this->load_image();
	});

	data_entry::initialize();
}

void cursor::set_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_filepath()) {
		return;
	}

	this->filepath = database::get()->get_graphics_filepath(filepath);
}

QCoro::Task<void> cursor::load_image()
{
	if (!this->image.isNull()) {
		//already loaded
		co_return;
	}

	this->image = QImage(path::to_qstring(this->get_filepath()));
	assert_throw(!this->image.isNull());

	const centesimal_int &scale_factor = preferences::get()->get_scale_factor();
	QImage scaled_image;

	co_await QtConcurrent::run([this, &scaled_image, &scale_factor]() {
		scaled_image = image::scale<QImage::Format_ARGB32>(this->image, scale_factor, this->image.size(), [](const size_t factor, const uint32_t *src, uint32_t *tgt, const int src_width, const int src_height) {
			xbrz::scale(factor, src, tgt, src_width, src_height, xbrz::ColorFormat::ARGB);
		});
	});

	this->image = std::move(scaled_image);
}

}
