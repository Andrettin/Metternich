import QtQuick
import QtQuick.Controls

Item {
	id: tile_image_item
	
	width: can_be_subtile ? tile_image.width : tile_size
	height: can_be_subtile ? tile_image.height : tile_size
	
	property string tile_image_source: ""
	property bool can_be_subtile: false
	
	Image {
		id: tile_image
		source: tile_image_source
		anchors.horizontalCenter: tile_image_item.horizontalCenter
		anchors.verticalCenter: tile_image_item.verticalCenter
		fillMode: Image.Pad
	}
}
