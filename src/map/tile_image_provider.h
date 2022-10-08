#pragma once

#include "util/image_provider_base.h"

namespace metternich {

class tile_image_provider final : public image_provider_base
{
public:
	virtual void load_image(const std::string &id) override;
};

}
