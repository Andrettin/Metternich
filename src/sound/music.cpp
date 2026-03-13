#include "metternich.h"

#include "sound/music.h"

#include "database/database.h"
#include "util/assert_util.h"

namespace metternich {

music::music(const std::string &identifier) : data_entry(identifier)
{
}

music::~music()
{
}

void music::check() const
{
	assert_throw(!this->get_filepath().empty());
}

void music::set_filepath(const std::filesystem::path &filepath)
{
	if (filepath == this->get_filepath()) {
		return;
	}

	this->filepath = database::get()->get_music_path(this->get_module()) / filepath;
}

}
