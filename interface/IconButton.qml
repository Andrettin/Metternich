import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

ButtonBase {
	id: button
	width: (32 * scale_factor) + 6 * scale_factor
	height: (32 * scale_factor) + 6 * scale_factor
	radius: circle ? (width * 0.5) : (width * 0.25)
	
	property string icon_identifier: ""
	property bool circle: false
	property bool use_opacity_mask: true
	
	Item {
		id: icon_image_area
		anchors.verticalCenter: parent.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		width: parent.width - 1 * scale_factor * 2 //width inside the border
		height: parent.height - 1 * scale_factor * 2
		layer.enabled: true
		visible: use_opacity_mask === false
		
		Image {
			id: icon_image
			anchors.verticalCenter: parent.verticalCenter
			anchors.verticalCenterOffset: button.down ? 1 * scale_factor : 0
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.horizontalCenterOffset: button.down ? 1 * scale_factor : 0
			source: "image://icon/" + icon_identifier
			fillMode: Image.Pad
		}
	}
	
	OpacityMask {
		id: opacity_mask_rectangle
        anchors.fill: icon_image_area
		width: icon_image_area.width
		height: icon_image_area.height
		radius: button.radius
        source: icon_image_area
		visible: use_opacity_mask
	}
}
