import QtQuick
import QtQuick.Controls

Image {
	id: flag_image
	source: flag.length > 0 ? ("file:graphics/flags/" + flag + ".svg") : ""
	width: 36 * scale_factor
	height: 24 * scale_factor
	sourceSize.width: 36 * scale_factor
	sourceSize.height: 24 * scale_factor
	fillMode: Image.Stretch
	
	property string flag: ""
}
